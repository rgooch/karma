KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/krenzo

all:	$(TARGETS)	generic_clean


KRENZO  = main_xt.c generic.c globals.c

KRENZOO = main_xt.o generic.o globals.o

$(KARMABINPATH)/krenzo:	$(KRENZOO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkrenzo $(KRENZOO) $(CLIBS)
	\rm -f $(KARMABINPATH)/krenzo
	install -s $(machine_dir)/tmpkrenzo $(KARMABINPATH)/krenzo
	chmod u=rwx,go=x $(KARMABINPATH)/krenzo


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KRENZO)


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
