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
from threading import Thread
from time import sleep
from os.path import join
from pydoorlock.AvrDoorlock import AvrDoorlockBackend
from pydoorlock.NukiBridge import NukiBridge
from pydoorlock.SimulationBackend import SimulationBackend

from .Config import Config

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
    BackendError = 5

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
        elif self == DoorlockResponse.BackendError:
            return "Backend Error"

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

    def __init__(self, cfg: Config, sounds_prefix, scripts_prefix):
        self._callback = None

        self.sounds = cfg.boolean('SOUNDS')
        if self.sounds:
            self.sounds_prefix = sounds_prefix

        self.scripts_prefix = scripts_prefix
        self.run_hooks = cfg.boolean('RUN_HOOKS')

        backend_type = cfg.str("BACKEND_TYPE", "backend")
        print(backend_type)
        if not backend_type:
            log.error("No backend configured")
            raise RuntimeError()

        if backend_type == "avr":
            self.backend = AvrDoorlockBackend(self)
        elif backend_type == "nuki":
            self.backend = NukiBridge(cfg.str("NUKI_ENDPOINT", "backend"),
                                      cfg.str("NUKI_APITOKEN", "backend"),
                                      cfg.str("NUKI_DEVICE", "backend"))
        elif backend_type == "simulation":
            self.backend = SimulationBackend(self)
        else:
            log.error(f"Unknown backend {backend_type}")
            raise RuntimeError

    def state_changed(self, new_state):
        if new_state == DoorState.Open:
            self.run_hook('post_unlock')
        elif new_state == DoorState.Present:
            self.run_hook('post_present')
        elif new_state == DoorState.Closed:
            self.run_hook('post_lock')
        else:
            return

        self.state = new_state


    def open(self):
        if self.state == DoorState.Open:
            return DoorlockResponse.AlreadyActive

        if self.backend.set_state(DoorState.Open):
            self.state = DoorState.Open
            self.run_hook('post_unlock')
            return DoorlockResponse.Success

        return DoorlockResponse.BackendError

    def present(self):
        if self.state == DoorState.Present:
            return DoorlockResponse.AlreadyActive

        if self.backend.set_state(DoorState.Present):
            self.state = DoorState.Present
            self.run_hook('post_present')
            return DoorlockResponse.Success

        return DoorlockResponse.BackendError

    def close(self):
        if self.state == DoorState.Closed:
            return DoorlockResponse.AlreadyActive

        if self.backend.set_state(DoorState.Closed):
            self.state = DoorState.Closed
            self.run_hook('post_lock')
            return DoorlockResponse.Success

        return DoorlockResponse.BackendError

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
