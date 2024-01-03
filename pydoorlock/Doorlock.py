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

from subprocess import run
from threading import Thread
from time import sleep
from os.path import join
from pydoorlock.AvrDoorlock import AvrDoorlockBackend
from pydoorlock.NukiBridge import NukiBridge
from pydoorlock.SimulationBackend import SimulationBackend

from .Config import Config
from .DoorlockResponse import DoorlockResponse
from .Door import DoorState
from .Protocol import Protocol

log = logging.getLogger()

def run_background(cmd):
    run('%s &' % cmd, shell=True)

class DoorHandler:
    state = DoorState.Closed

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

        self.backend.set_state(self.state)
        self.backend.register_state_changed_handler(self.backend_state_change_handler)

    def backend_state_change_handler(self, new_state):
        self.update_state(self.state, new_state, DoorlockResponse.Success)

    def open(self):
        if self.state == DoorState.Open:
            return DoorlockResponse.AlreadyActive

        if self.backend.set_state(DoorState.Open):
            self.state = DoorState.Open
            return DoorlockResponse.Success

        return DoorlockResponse.BackendError

    def present(self):
        if self.state == DoorState.Present:
            return DoorlockResponse.AlreadyActive

        if self.backend.set_state(DoorState.Present):
            self.state = DoorState.Present
            return DoorlockResponse.Success

        return DoorlockResponse.BackendError

    def close(self):
        if self.state == DoorState.Closed:
            return DoorlockResponse.AlreadyActive

        if self.backend.set_state(DoorState.Closed):
            self.state = DoorState.Closed
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

        self.update_state(old_state, state, err)

        return err

    def update_state(self, old_state, new_state, response = DoorlockResponse.Success):
        if (old_state != new_state) and response == DoorlockResponse.Success:
            if new_state == DoorState.Open:
                self.run_hook('post_unlock')
            elif new_state == DoorState.Closed:
                self.run_hook('post_lock')
            elif new_state == DoorState.Present:
                self.run_hook('post_present')

        self.state = new_state
        self.sound_helper(old_state, new_state, False)
        self.invoke_callback(response)

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
