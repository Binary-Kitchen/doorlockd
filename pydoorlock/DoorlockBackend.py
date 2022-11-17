from abc import ABC, abstractmethod


class DoorlockBackend(ABC):
    def __init__(self):
        self.callbacks = list()
        self.current_state = None

    def update_state(self):
        self.state_change_callback(self.current_state)

    @abstractmethod
    def set_state(self, state):
        self.current_state = state

    @abstractmethod
    def get_state(self, state):
        return self.current_state
