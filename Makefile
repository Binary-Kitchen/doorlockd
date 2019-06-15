DESTDIR ?= /
PREFIX ?= /usr
SYSCONFDIR ?= /etc

USR = $(DESTDIR)/$(PREFIX)
BIN = $(USR)/bin/
ETC = $(DESTDIR)/etc/
SHARE = $(USR)/share/
SYSTEMD_UNITS = $(ETC)/systemd/system/

all: gpio-wait pydoorlock/Protocol.py

package:
	sed -i -r -e "s@(^SYSCONFDIR = ').*('$$)@\1$(SYSCONFDIR)\2@" pydoorlock/Config.py
	sed -i -r -e "s@(^PREFIX = ').*('$$)@\1$(PREFIX)\2@" pydoorlock/Config.py
	sed -i -r -e "s@(^__version__ = ').*('$$)@\1$(shell cat VERSION)\2@" doorlockd

pydoorlock/Protocol.py: avr-code/protocol.h
	./scripts/gen_protocol.sh $^ > $@

gpio-wait: gpio-wait.c

install:
	mkdir -p $(BIN)
	mkdir -p $(SHARE)
	mkdir -p $(SYSTEMD_UNITS)
	mkdir -p $(ETC)

	install doorlockd gpio-wait doorstate $(BIN)
	install doorlockd-passwd $(BIN)
	install -m 0644 etc/doorlockd.cfg $(ETC)
	install -m 0644 systemd/doorlockd.service systemd/doorstate.service $(SYSTEMD_UNITS)

	pip install --upgrade --force-reinstall --root=$(DESTDIR) .

	cp -av share/* $(SHARE)

clean:
	rm -f gpio-wait
	rm -rf pydoorlock/__pycache__
	rm -f pydoorlock/Protocol.py
