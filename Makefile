DESTDIR ?= /
PREFIX ?= /usr
SYSCONFDIR ?= /etc

all: gpio-wait
	sed -i -r -e "s@(^SYSCONFDIR = ').*('$$)@\1$(SYSCONFDIR)\2@" doorlockd
	sed -i -r -e "s@(^PREFIX = ').*('$$)@\1$(PREFIX)\2@" doorlockd

gpio-wait: gpio-wait.c

install:
	mkdir -p $(DESTDIR)/$(PREFIX)/bin/
	mkdir -p $(DESTDIR)/$(PREFIX)/share/
	mkdir -p $(DESTDIR)/$(SYSCONFDIR)/systemd/system
	mkdir -p $(DESTDIR)/$(SYSCONFDIR)

	install doorlockd gpio-wait doorstate $(DESTDIR)/$(PREFIX)/bin/
	install doorlockd-passwd $(DESTDIR)/$(PREFIX)/bin/
	install -m 0644 doorlockd.cfg $(DESTDIR)/$(SYSCONFDIR)
	install -m 0644 doorlockd.service doorstate.service $(DESTDIR)/$(SYSCONFDIR)/systemd/system

	cp -av share/* $(DESTDIR)/$(PREFIX)/share
