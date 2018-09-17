DESTDIR ?= /
PREFIX ?= /usr
SYSCONFDIR ?= /etc

all:
	sed -i -r -e "s@(^SYSCONFDIR = ').*('$$)@\1$(SYSCONFDIR)\2@" doorlockd
	sed -i -r -e "s@(^PREFIX = ').*('$$)@\1$(PREFIX)\2@" doorlockd

install:
	mkdir -p $(DESTDIR)/$(PREFIX)/bin/
	mkdir -p $(DESTDIR)/$(PREFIX)/share/
	mkdir -p $(DESTDIR)/$(SYSCONFDIR)/systemd/system
	mkdir -p $(DESTDIR)/$(SYSCONFDIR)

	install doorlockd $(DESTDIR)/$(PREFIX)/bin/
	install doorlockd-passwd $(DESTDIR)/$(PREFIX)/bin/
	install -m 0644 doorlockd.cfg $(DESTDIR)/$(SYSCONFDIR)
	install -m 0644 doorlockd.service $(DESTDIR)/$(SYSCONFDIR)/systemd/system

	cp -av share/* $(DESTDIR)/$(PREFIX)/share
