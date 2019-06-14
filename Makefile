DESTDIR ?= /
PREFIX ?= /usr
SYSCONFDIR ?= /etc

all: gpio-wait pydoorlock/Protocol.py

package:
	sed -i -r -e "s@(^SYSCONFDIR = ').*('$$)@\1$(SYSCONFDIR)\2@" doorlockd
	sed -i -r -e "s@(^PREFIX = ').*('$$)@\1$(PREFIX)\2@" doorlockd
	sed -i -r -e "s@(^__version__ = ').*('$$)@\1$(shell cat VERSION)\2@" doorlockd

pydoorlock/Protocol.py: avr-code/protocol.h
	./scripts/gen_protocol.sh $^ > $@

gpio-wait: gpio-wait.c

install:
	mkdir -p $(DESTDIR)/$(PREFIX)/bin/
	mkdir -p $(DESTDIR)/$(PREFIX)/share/
	mkdir -p $(DESTDIR)/$(SYSCONFDIR)/systemd/system
	mkdir -p $(DESTDIR)/$(SYSCONFDIR)

	install doorlockd gpio-wait doorstate $(DESTDIR)/$(PREFIX)/bin/
	install doorlockd-passwd $(DESTDIR)/$(PREFIX)/bin/
	install -m 0644 doorlockd.default.cfg doorlockd.cfg $(DESTDIR)/$(SYSCONFDIR)
	install -m 0644 doorlockd.service doorstate.service $(DESTDIR)/$(SYSCONFDIR)/systemd/system

	pip install --upgrade --force-reinstall --root=$(DESTDIR) .

	cp -av share/* $(DESTDIR)/$(PREFIX)/share

clean:
	rm -f gpio-wait
	rm -rf pydoorlock/__pycache__
	rm -f pydoorlock/Protocol.py
