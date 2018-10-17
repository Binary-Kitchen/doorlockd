import requests
import threading
import json

class EventClient:
    def __init__(self,url):
        self.r = requests.get(url,stream=True)
        print("connected")
        if self.r.encoding is None:
            self.r.encoding = 'utf8'

    def parseEvent(self,line_buf):
        ret = dict()
        for line in line_buf:
            if line:
                k,v = line.decode().split(':',1)
                ret[k] = v
        return ret
        
    def events(self):
        lines = self.r.iter_lines()
        line_buf = []
        while True:
            line_buf.append(next(lines))
            if line_buf:
                if not line_buf[-1].decode():
                    yield self.parseEvent(line_buf)
                    line_buf = []

if __name__ == "__main__":
    e = EventClient("http://localhost:8080/push")
    for evt in e.events():
        print(json.loads(evt['data']))

