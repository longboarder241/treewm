# recreated<08.02.2003> by Rudolf Polzer: should be easier
#   to use and FreeBSD conforming

# options for the user (prefer to set them via the make command)

PREFIX   = /usr/local
ROOT     =
BINDIR   = $(PREFIX)/bin
SHARE    = $(PREFIX)/share/treewm
DOC      = $(PREFIX)/share/doc/treewm
CXXFLAGS = -O2
TARGETS  = treewm xprop xkill
DEFINES  = -DSHAPE -DVIDMODE -DPIXMAPS=\"$(PIXMAPS)/\" #-DDEBUG

# internals (change them if needed, but probably you won't)

VERSION  = 0.4.5

LIBS     = -L/usr/X11R6/lib -lXxf86vm -lXpm -lXext -lX11 #-lelf -lbfd -lmpatrol
INCLUDES = -I/usr/X11R6/include

CCFILES = $(wildcard src/*.cc)
OFILES = $(patsubst %.cc,%.o,$(CCFILES))

PIXMAPS = $(SHARE)/pixmaps
DOCFILES = AUTHORS COPYING ChangeLog INSTALL PROBLEMS README README.tiling TODO default.cfg sample.cfg
PIXMAPFILES = $(wildcard src/pixmaps/*.xpm)
BINFILES = src/treewm
BINFILES_OPTIONAL = xprop/xprop xkill/xkill
BINFILES_OPTIONAL_SUFFIX = -treewm

all: $(TARGETS)

.PHONY: all clean distclean install installonly uninstall treewm xkill xprop snapshot targz tarbz2

clean:
	$(RM) $(OFILES) src/treewm
	! [ -f xkill/Makefile ] || $(MAKE) -C xkill $@
	! [ -f xprop/Makefile ] || $(MAKE) -C xprop $@

distclean: clean
	! [ -f xkill/Makefile ] || $(MAKE) -C xkill $@
	! [ -f xprop/Makefile ] || $(MAKE) -C xprop $@

install: all installonly

installonly:
	install -d $(ROOT)$(BINDIR) $(ROOT)$(PIXMAPS) $(ROOT)$(DOC)
	install $(BINFILES) $(ROOT)$(BINDIR)/
	for PROG in $(BINFILES_OPTIONAL); do \
	  NAME="`basename $$PROG`"; \
	  install $$PROG $(ROOT)$(BINDIR)/$$NAME$(BINFILES_OPTIONAL_SUFFIX) || true; \
	done
	install -m 644 $(DOCFILES) $(ROOT)$(DOC)/
	install -m 644 $(PIXMAPFILES) $(ROOT)$(PIXMAPS)/

uninstall:
	cd $(ROOT)$(PIXMAPS); for PIXMAP in $(PIXMAPFILES); do \
	  NAME="`basename $$PIXMAP`"; \
	  $(RM) $$NAME; \
	done
	cd $(ROOT)$(DOC); for DOC in $(DOCFILES); do \
	  NAME="`basename $$DOC`"; \
	  $(RM) $$NAME; \
	done
	cd $(ROOT)$(BINDIR); for PROG in $(BINFILES_OPTIONAL); do \
	  NAME="`basename $$PROG`"; \
	  $(RM) $$NAME$(BINFILES_OPTIONAL_SUFFIX); \
	done
	cd $(ROOT)$(BINDIR); for PROG in $(BINFILES); do \
	  NAME="`basename $$PROG`"; \
	  $(RM) $$NAME; \
	done
	rmdir $(ROOT)$(PIXMAPS) || true
	rmdir $(ROOT)$(SHARE) || true
	rmdir $(ROOT)$(DOC) || true

treewm: src/treewm

xkill xprop:
	cd $@; [ -f Makefile ] || xmkmf; $(MAKE) all

snapshot: distclean
	DIR="`pwd`"; DIR="`basename "$$DIR"`"; cd ..; tar czf treewm-`date +%Y%m%d`.tar.gz "$$DIR"

targz: distclean
	DIR="`pwd`"; DIR="`basename "$$DIR"`"; cd ..; find "$$DIR" -name .cvsignore -o -name CVS |\
	  tar czf treewm-$(VERSION).tar.gz -X - "$$DIR"

tarbz2: distclean
	DIR="`pwd`"; DIR="`basename "$$DIR"`"; cd ..; find "$$DIR" -name .cvsignore -o -name CVS |\
	  tar cjf treewm-$(VERSION).tar.bz2 -X - "$$DIR"

src/treewm: $(OFILES)
	$(CXX) $(LDFLAGS)  -o src/treewm $(OFILES) $(LIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -o $@ -c $<
