"""
Doorlockd -- Binary Kitchen's smart door opener

Copyright (c) Binary Kitchen e.V., 2022

Author:
  Thomas Schmid <tom@binary-kitchen.de>

This work is licensed under the terms of the GNU GPL, version 2.  See
the LICENSE file in the top-level directory.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
"""

from abc import ABC, abstractmethod


class DoorlockBackend(ABC):
    def __init__(self):
        self.callbacks = list()
        self.current_state = None

    def register_state_changed_handler(self, callback):
        self.state_change_callback = callback

    @abstractmethod
    def set_state(self, state):
        self.current_state = state

    @abstractmethod
    def get_state(self, state):
        return self.current_state
