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
