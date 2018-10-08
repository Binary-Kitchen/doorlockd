import logging

from enum import Enum
from random import sample
from subprocess import Popen
from serial import Serial
from threading import Thread
from time import sleep
from os.path import join

from .Door import DoorState

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
    return(sample(eperm_insults, 1)[0])


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

    CMD_PRESENT = b'y'
    CMD_OPEN = b'g'
    CMD_CLOSE = b'r'

    BUTTON_PRESENT = b'Y'
    BUTTON_OPEN = b'G'
    BUTTON_CLOSE = b'R'

    CMD_EMERGENCY_SWITCH = b'E'

    wave_lock = 'lock.wav'
    wave_lock_button = 'lock_button.wav'

    wave_present = 'present.wav'
    wave_present_button = 'present.wav'

    wave_unlock = 'unlock.wav'
    wave_unlock_button = 'unlock_button.wav'

    wave_zonk = 'zonk.wav'

    def __init__(self, cfg, sounds_prefix, scripts_prefix):
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
                if rx == DoorHandler.BUTTON_CLOSE:
                    self.close()
                    log.info('Closed due to Button press')
                    #emit_status(LogicResponse.ButtonLock)
                elif rx == DoorHandler.BUTTON_OPEN:
                    self.open()
                    log.info('Opened due to Button press')
                    #emit_status(LogicResponse.ButtonUnlock)
                elif rx == DoorHandler.BUTTON_PRESENT:
                    self.present()
                    log.info('Present due to Button press')
                    #emit_status(LogicResponse.ButtonPresent)
                elif rx == DoorHandler.CMD_EMERGENCY_SWITCH:
                    log.warning('Emergency unlock')
                    #emit_status(LogicResponse.EmergencyUnlock)
                else:
                    log.error('Received unknown message "%s" from AVR' % rx)

                self.sound_helper(old_state, self.state, True)

            if self.do_close:
                tx = DoorHandler.CMD_CLOSE
                self.do_close = False
            elif self.state == DoorState.Present:
                tx = DoorHandler.CMD_PRESENT
            elif self.state == DoorState.Open:
                tx = DoorHandler.CMD_OPEN
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
        #emit_doorstate()
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

        Popen(['nohup', 'aplay', join(self.sounds_prefix, filename)])

    def run_hook(self, script):
        if not self.run_hooks:
            log.info('Hooks disabled: not starting %s' % script)
            return
        log.info('Starting hook %s' % script)
        Popen(['nohup', join(self.scripts_prefix, script)])
