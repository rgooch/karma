KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/kpvslice

all:	$(TARGETS)	generic_clean


KPVSLICE  = main_xt.c generic.c event.c pvget.c globals.c

KPVSLICEO = main_xt.o generic.o event.o pvget.o globals.o

$(KARMABINPATH)/kpvslice:	$(KPVSLICEO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkpvslice $(KPVSLICEO) $(CLIBS)
	\rm -f $(KARMABINPATH)/kpvslice
	install -s $(machine_dir)/tmpkpvslice $(KARMABINPATH)/kpvslice
	chmod u=rwx,go=x $(KARMABINPATH)/kpvslice


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KPVSLICE)


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
