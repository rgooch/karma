include ../GNUmakefile.head

ifeq ($(OS),Solaris)
LD = $(KARMA_LD) -R$(KARMALIBPATH):$(XLIBPATH)
endif

ifeq ($(OS),IRIX5)
LD = $(KARMA_LD) -rpath $(KARMALIBPATH):$(XLIBPATH)
endif

ifeq ($(OS),OSF1)
LD = $(KARMA_LD) -Wl,-rpath,$(KARMALIBPATH):$(XLIBPATH)
endif

CFLAGS = $(COPTIMISE) -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -DOS_$(OS) -DMACHINE_$(MACHINE) -DX11
KARMALIBS = -L$(KARMALIBPATH)	-lkarmawidgets -lkarmagraphics -lkarmaXt \
				-lkarmaX11 -lkarma
XLIBS = -L$(XLIBPATH) -lXaw -lXmu -lXt -lXext -lX11


TARGETS =	$(KARMABINPATH)/kmovie

all:	$(TARGETS)	clean


KMOVIE =	kmovie_main.c kmovie_generic.c

KMOVIEO =	kmovie_main.o kmovie_generic.o

$(KARMABINPATH)/kmovie:	$(KMOVIEO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkmovie $(KMOVIEO) $(CLIBS)
	install -s $(machine_dir)/tmpkmovie $(KARMABINPATH)/kmovie
	chmod u=rwx,go=x $(KARMABINPATH)/kmovie


depend:
	makedepend -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KMOVIE)



clean:
	rm -f *.BAK *.log *.sed date.h .#* *~ *.bak */*.bak */tmp* .*defaults


remove:
	\rm -f $(TARGETS)

include $(machine_dir)/depend
