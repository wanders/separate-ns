
DESTDIR:=/
PREFIX:=/usr/local
INSTALL:=install

separate-ns: separate-ns.c
	$(CC) -Wall -Wextra -Wno-sign-compare -Wwrite-strings -o $@ $<

.PHONY: install
install: separate-ns
	$(INSTALL) -d $(DESTDIR)$(PREFIX)
	$(INSTALL) --mode 4755 separate-ns $(DESTDIR)$(PREFIX)/bin

.PHONY: clean
clean:
	rm -f separate-ns
