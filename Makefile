
DESTDIR:=/
PREFIX:=/usr/local
INSTALL:=install

separate-ns: separate-ns.c
	$(CC) -Wall -Wextra -Wno-sign-compare -Wwrite-strings -o $@ $<

.PHONY: install
install: separate-ns
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) --mode 4755 separate-ns $(DESTDIR)$(PREFIX)/bin

.PHONY: clean
clean:
	rm -f separate-ns

.PHONY: dist
dist: VER:=$(shell date +%Y%m%d)
dist:
	mkdir separate-ns-$(VER)
	cp COPYING Makefile README.org separate-ns.c separate-ns-$(VER)/
	tar czf separate-ns-$(VER).tar.gz separate-ns-$(VER)/
	rm separate-ns-$(VER)/*
	rmdir separate-ns-$(VER)
