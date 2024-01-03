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

import json
import logging
import threading
import time
from urllib.parse import urljoin

import requests

from .Door import DoorState
from .DoorlockBackend import DoorlockBackend
from .DoorlockResponse import DoorlockResponse

log = logging.getLogger(__name__)
reqlog = logging.getLogger('urllib3')
reqlog.setLevel(logging.WARNING)

def compare_dicts(d1, d2, ignore_keys):
    ka = set(d1).difference(ignore_keys)
    kb = set(d2).difference(ignore_keys)

    return ka == kb and all(d1[k] == d2[k] for k in ka)

class NukiBridgeDevice():
    """
    class to interface with a nuki smartlock
    """
    def __init__(self, endpoint: str, apitoken: str, device_name: str) -> None:
        self.endpoint = endpoint
        self.apitoken = apitoken

        devices = self.list()

        if devices is None:
            log.error("Unable to get device id")
            raise RuntimeError("unable to get nuki device id")
        for device in devices:
            if device["name"] == device_name:
                self.device_id = device["nukiId"]
    
    def get_device_id(self):
        return self.device_id

    def make_request(self, endpoint, parameters: dict =  {}):
        url = urljoin(self.endpoint, endpoint)
        parameters["token"] = self.apitoken

        try:
            return requests.get(url, parameters)
        except Exception as e:
            log.error(f"{e}")
            return None
        
    def info(self):
        response = self.make_request("info")
        return response

    def list(self):
        response = self.make_request("list")
        
        if response is None:
            return None

        return json.loads(response.content)

    def get_device_state(self):
        response = self.list()

        if response is None:
            return None

        for info in response:
                if info["nukiId"] == self.get_device_id():
                    return info["lastKnownState"]

    def compare_device_state(state1, state2):
        return compare_dicts(state1, state2, ["timestamp"])

    def lock(self, device_name: str = None): 
        if device_name == None:
            nukiId = self.device_id
        else:
            RuntimeError("")

        response = self.make_request("lock", {"nukiid": nukiId})
        
        if response is None:
            return False

        if response.status_code == 200:
            json_response = json.loads(response.content)
            return json_response["success"]

        log.error(f"Nuki responded with an error: {json_response}")
        return False

    def unlock(self, device_name: str = None):
        if device_name == None:
            nukiId = self.device_id
        else:
            RuntimeError("")

        response = self.make_request("unlock", {"nukiid": nukiId})

        if response is None:
            return False

        if response.status_code == 200:
            json_response = json.loads(response.content)
            return json_response["success"]
        return False


class NukiBridge(DoorlockBackend):
    def __init__(self, endpoint, api_key, device_name):
        #self.device = NukiBridgeDevice(endpoint,api_key, device_name)
        self.device = NukiBridgeDevice(endpoint, api_key, device_name)
        self.poll_thread = threading.Thread(target=self.poll_worker)
        self.poll_thread.start()
        self.current_state = DoorState.Closed

    def poll_worker(self):
        last_dev_state = self.device.get_device_state()

        while True:
            dev_state = self.device.get_device_state()

            if dev_state is None:
                continue

            if not NukiBridgeDevice.compare_device_state(dev_state, last_dev_state):
                log.info(f"Nuki changed state: {dev_state}")

                if self.current_state != DoorState.Closed and dev_state["stateName"] == "locked":
                    self.current_state = DoorState.Closed
                    self.state_change_callback(self.current_state)

                if self.current_state != DoorState.Open and dev_state["stateName"] == "unlocked":
                    self.current_state = DoorState.Open
                    self.state_change_callback(self.current_state)

            last_dev_state = dev_state
            time.sleep(10)

    def set_state(self, state):
        success = False

        if state == DoorState.Open:
            log.info("open nuki")
            if self.device.unlock():
                self.current_state = DoorState.Open
                success = True
        elif state == DoorState.Closed:
            log.info("close nuki")
            if self.device.lock():
                self.current_state = DoorState.Closed
                success = True

        return success

    def get_state(self, state):
        return self.current_state
