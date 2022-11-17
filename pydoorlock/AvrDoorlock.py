import logging
import threading
from time import sleep

from serial import Serial

from .Door import DoorState
from .DoorlockBackend import DoorlockBackend
from .Protocol import Protocol

log = logging.getLogger(__name__)

class AvrDoorlockBackend(DoorlockBackend):
    def __init__(self, device):
        self.serial = Serial(device, baudrate=9600, bytesize=8, parity='N',
                             stopbits=1, timeout=0)
        self.do_close = False
        self.do_open = False
        self.do_present = False

        threading.Thread(target=self.thread_worker).start()

    def get_capabilities(self):
        return [DoorState.Closed, DoorState.Open, DoorState.Present]

    def set_state(self, state):
        if state == DoorState.Closed:
            self.do_close = True
        elif state == DoorState.Open:
            self.do_open = True
        elif state == DoorState.Present:
            self.do_present = True

    def thread_worker(self):
        while True:
            sleep(0.4)
            while True:
                rx = self.serial.read(1)
                if len(rx) == 0:
                    break

                old_state = self.state
                if rx == Protocol.STATE_SWITCH_RED.value.upper():
                    self.door_handler.close()
                    log.info('Closed due to Button press')
                    self.door_handler.invoke_callback(DoorlockResponse.ButtonLock)
                elif rx == Protocol.STATE_SWITCH_GREEN.value.upper():
                    self.door_handler.open()
                    log.info('Opened due to Button press')
                    self.door_handler.invoke_callback(DoorlockResponse.ButtonUnlock)
                elif rx == Protocol.STATE_SWITCH_YELLOW.value.upper():
                    self.door_handler.present()
                    log.info('Present due to Button press')
                    self.door_handler.invoke_callback(DoorlockResponse.ButtonPresent)
                elif rx == Protocol.EMERGENCY.value:
                    log.warning('Emergency unlock')
                    self.door_handler.invoke_callback(DoorlockResponse.EmergencyUnlock)
                else:
                    log.error('Received unknown message "%s" from AVR' % rx)

                self.sound_helper(old_state, self.state, True)

            if self.do_close:
                tx = Protocol.STATE_SWITCH_RED.value
                self.do_close = False
            elif self.do_present:
                tx = Protocol.STATE_SWITCH_YELLOW.value
                self.do_present = False
            elif self.do_open:
                tx = Protocol.STATE_SWITCH_GREEN.value
                self.do_open = False
            else:
                continue

            self.serial.write(tx)
            self.serial.flush()
