import requests
import threading
import json

class EventClient:
    def __init__(self,url, chunk_size = 1024):
        self.chunk_size = chunk_size
        self.r = requests.get(url,stream=True)
        if self.r.encoding is None:
            self.r.encoding = 'utf8'

    def parseEvent(self, raw):
        lines = raw.splitlines()
        data = dict()
        for line in lines:
            k,v = line.split(":",1)
            if data.get(k):
                data[k] += v
            data[k] = v
        return data

    def events(self):
        def generate():
            sbuf = ""
            while True:
                bbuf = self.r.raw._fp.fp.read1(self.chunk_size)

                if not bbuf:
                    break

                sbuf += bbuf.decode('utf8')
                parts = sbuf.split('\n\n')
                if len(parts) > 1:
                    for p in parts[0:-1]:
                        yield self.parseEvent(p)

                sbuf = parts[-1]
        return generate()

if __name__ == "__main__":
    while True:
        e = EventClient("http://localhost:5000/push")
        for evt in e.events():
            print(json.loads(evt['data']))

