from .DoorlockBackend import DoorlockBackend


class SimulationBackend(DoorlockBackend):
    def __init__(self, handler):
        super.__init__(handler)
