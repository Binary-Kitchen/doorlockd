"""
Doorlockd -- Binary Kitchen's smart door opener

Copyright (c) Binary Kitchen e.V., 2018

Author:
  Ralf Ramsauer <ralf@binary-kitchen.de>

This work is licensed under the terms of the GNU GPL, version 2.  See
the LICENSE file in the top-level directory.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
"""

from enum import Enum


class DoorState(Enum):
    Open = 0
    Present = 1
    Closed = 2

    def from_string(string):
        if string == 'lock':
            return DoorState.Closed
        elif string == 'unlock':
            return DoorState.Open
        elif string == 'present':
            return DoorState.Present

        return None

    def is_open(self):
        if self != DoorState.Closed:
            return True
        return False

    def to_img(self):
        led = 'red'
        if self == DoorState.Present:
            led = 'yellow'
        elif self == DoorState.Open:
            led = 'green'
        return 'static/led-%s.png' % led

    def __str__(self):
        if self == DoorState.Open:
            return 'Offen'
        elif self == DoorState.Present:
            return 'Jemand da'
        return 'Geschlossen'
