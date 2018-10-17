"""
Doorlockd -- Binary Kitchen's smart door opener

Copyright (c) Binary Kitchen e.V., 2018-2019

Author:
  Ralf Ramsauer <ralf@binary-kitchen.de>

This work is licensed under the terms of the GNU GPL, version 2.  See
the LICENSE file in the top-level directory.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
"""

import logging

from enum import Enum
from random import sample
from subprocess import run
from serial import Serial
from threading import Thread
from time import sleep
from os.path import join
import gevent
from .Door import DoorState
from .Protocol import Protocol

log = logging.getLogger()

# copied from sudo
eperm_insults = {
        'Wrong!  You cheating scum!',
        'And you call yourself a Rocket Scientist!',
        'No soap, honkie-lips.',
        'Where did you learn to type?',
        'Are you on drugs?',
        'My pet ferret can type better than you!',
        'You type like i drive.',
        'Do you think like you type?',
        'Your mind just hasn\'t been the same since the electro-shock, has it?',
        'Maybe if you used more than just two fingers...',
        'BOB says:  You seem to have forgotten your passwd, enter another!',
        'stty: unknown mode: doofus',
        'I can\'t hear you -- I\'m using the scrambler.',
        'The more you drive -- the dumber you get.',
        'Listen, broccoli brains, I don\'t have time to listen to this trash.',
        'I\'ve seen penguins that can type better than that.',
        'Have you considered trying to match wits with a rutabaga?',
        'You speak an infinite deal of nothing',
}


def choose_insult():
    return sample(eperm_insults, 1)[0]


def run_background(cmd):
    run('%s &' % cmd, shell=True)


class DoorlockResponse(Enum):
    Success = 0
    Perm = 1
    AlreadyActive = 2
    # don't break old apps, value 3 is reserved now
    RESERVED = 3
    Inval = 4

    EmergencyUnlock = 10,
    ButtonLock = 11,
    ButtonUnlock = 12,
    ButtonPresent = 13,

    def __str__(self):
        if self == DoorlockResponse.Success:
            return 'Yo, passt.'
        elif self == DoorlockResponse.Perm:
            return choose_insult()
        elif self == DoorlockResponse.AlreadyActive:
            return 'Zustand bereits aktiv'
        elif self == DoorlockResponse.Inval:
            return 'Das was du willst geht nicht.'
        elif self == DoorlockResponse.EmergencyUnlock:
            return '!!! Emergency Unlock !!!'
        elif self == DoorlockResponse.ButtonLock:
            return 'Closed by button'
        elif self == DoorlockResponse.ButtonUnlock:
            return 'Opened by button'
        elif self == DoorlockResponse.ButtonPresent:
            return 'Present by button'

        return 'Error'


class DoorHandler:
    state = DoorState.Closed
    do_close = False

    wave_lock = 'lock.wav'
    wave_lock_button = 'lock_button.wav'

    wave_present = 'present.wav'
    wave_present_button = 'present.wav'

    wave_unlock = 'unlock.wav'
    wave_unlock_button = 'unlock_button.wav'

    wave_zonk = 'zonk.wav'

    def __init__(self, cfg, sounds_prefix, scripts_prefix):
        self._callback = None

        self.sounds = cfg.boolean('SOUNDS')
        if self.sounds:
            self.sounds_prefix = sounds_prefix

        self.scripts_prefix = scripts_prefix
        self.run_hooks = cfg.boolean('RUN_HOOKS')

        if cfg.boolean('SIMULATE_SERIAL'):
            return

        device = cfg.str('SERIAL_PORT')
        log.info('Using serial port: %s' % device)

        self.serial = Serial(device, baudrate=9600, bytesize=8, parity='N',
                             stopbits=1, timeout=0)
        self.thread = Thread(target=self.thread_worker)
        log.debug('Spawning RS232 Thread')
        self.thread.start()

    def thread_worker(self):
        while True:
            sleep(0.4)
            while True:
                rx = self.serial.read(1)
                if len(rx) == 0:
                    break

                old_state = self.state
                if rx == Protocol.STATE_SWITCH_RED.value.upper():
                    self.close()
                    log.info('Closed due to Button press')
                    self.invoke_callback(DoorlockResponse.ButtonLock)
                elif rx == Protocol.STATE_SWITCH_GREEN.value.upper():
                    self.open()
                    log.info('Opened due to Button press')
                    self.invoke_callback(DoorlockResponse.ButtonUnlock)
                elif rx == Protocol.STATE_SWITCH_YELLOW.value.upper():
                    self.present()
                    log.info('Present due to Button press')
                    self.invoke_callback(DoorlockResponse.ButtonPresent)
                elif rx == Protocol.EMERGENCY.value:
                    log.warning('Emergency unlock')
                    self.invoke_callback(DoorlockResponse.EmergencyUnlock)
                else:
                    log.error('Received unknown message "%s" from AVR' % rx)

                self.sound_helper(old_state, self.state, True)

            if self.do_close:
                tx = Protocol.STATE_SWITCH_RED.value
                self.do_close = False
            elif self.state == DoorState.Present:
                tx = Protocol.STATE_SWITCH_YELLOW.value
            elif self.state == DoorState.Open:
                tx = Protocol.STATE_SWITCH_GREEN.value
            else:
                continue

            self.serial.write(tx)
            self.serial.flush()

    def open(self):
        if self.state == DoorState.Open:
            return DoorlockResponse.AlreadyActive

        self.state = DoorState.Open
        self.run_hook('post_unlock')
        return DoorlockResponse.Success

    def present(self):
        if self.state == DoorState.Present:
            return DoorlockResponse.AlreadyActive

        self.state = DoorState.Present
        self.run_hook('post_present')
        return DoorlockResponse.Success

    def close(self):
        if self.state == DoorState.Closed:
            return DoorlockResponse.AlreadyActive

        self.do_close = True
        self.state = DoorState.Closed
        self.run_hook('post_lock')
        return DoorlockResponse.Success

    def request(self, state):
        old_state = self.state
        if state == DoorState.Closed:
            err = self.close()
        elif state == DoorState.Present:
            err = self.present()
        elif state == DoorState.Open:
            err = self.open()

        self.sound_helper(old_state, self.state, False)
        self.invoke_callback(err)
        return err

    def sound_helper(self, old_state, new_state, button):
        if not self.sounds:
            return

        # TBD: Emergency Unlock
        # wave_emergency = 'emergency_unlock.wav'

        if old_state == new_state:
            filename = self.wave_zonk
        elif button:
            if new_state == DoorState.Open:
                filename = self.wave_unlock_button
            elif new_state == DoorState.Present:
                filename = self.wave_present_button
            elif new_state == DoorState.Closed:
                filename = self.wave_lock_button
        else:
            if new_state == DoorState.Open:
                filename = self.wave_unlock
            elif new_state == DoorState.Present:
                filename = self.wave_present
            elif new_state == DoorState.Closed:
                filename = self.wave_lock

        run_background('aplay %s' % join(self.sounds_prefix, filename))

    def run_hook(self, script):
        if not self.run_hooks:
            log.info('Hooks disabled: not starting %s' % script)
            return
        log.info('Starting hook %s' % script)
        run_background(join(self.scripts_prefix, script))

    def register_callback(self, callback):
        self._callback = callback

    def invoke_callback(self, val):
        if self._callback:
            self._callback(val)
