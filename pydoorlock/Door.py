from enum import Enum

class DoorState(Enum):
    # These numbers are used by the App since version 3.0, do NOT change them
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

    def to_html(self):
        if self == DoorState.Open:
            return 'Offen'
        elif self == DoorState.Present:
            return 'Jemand da'
        return 'Geschlossen'
