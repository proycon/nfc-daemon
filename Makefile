SUBDIRS = src
export PREFIX = /usr/local

.PHONY: subdirs $(SUBDIRS)
.PHONY: all install uninstall

subdirs: $(SUBDIRS)
all: $(SUBDIRS)
$(SUBDIRS): 
	$(MAKE) -C $@

install:
	$(MAKE) -C src -f Makefile install

uninstall:
	$(MAKE) -C src -f Makefile uninstall

.PHONY: clean 
clean:
	$(MAKE) -C src -f Makefile clean
