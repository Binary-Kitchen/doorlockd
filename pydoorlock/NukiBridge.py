import json
import requests
from urllib.parse import urljoin
from .Door import DoorState
from .DoorlockBackend import DoorlockBackend
import logging
import threading
import time
from typing import Set

log = logging.getLogger(__name__)

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

    def poll_worker(self):
        while True:
            state = self.device.get_device_state()

            if state is None:
                continue

            self.current_state = state
            print(state)
            time.sleep(10)

    def set_state(self, state):
        if state == DoorState.Open:
            log.info("open nuki")
            return self.device.unlock()
        elif state == DoorState.Closed:
            log.info("close nuki")
            return self.device.lock()

    def get_state(self, state):
        return self.current_state
