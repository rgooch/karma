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


TARGETS =	$(KARMABINPATH)/kview_2d


all:	$(TARGETS)	clean


KVIEW_2D =	kview_2d_main.c kview_2d_generic.c

KVIEW_2DO =	kview_2d_main.o kview_2d_generic.o

$(KARMABINPATH)/kview_2d:	$(KVIEW_2DO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkview_2d $(KVIEW_2DO) $(CLIBS)
	install -s $(machine_dir)/tmpkview_2d $(KARMABINPATH)/kview_2d
	chmod u=rwx,go=x $(KARMABINPATH)/kview_2d


depend:
	makedepend -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KVIEW_2D)



clean:
	rm -f *.BAK *.log *.sed date.h .#* *~ *.bak */*.bak */tmp* .*defaults


remove:
	\rm -f $(TARGETS)

include $(machine_dir)/depend
