KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/kcolourmap

all:	$(TARGETS)	generic_clean


KCOLOURMAP =	main_xt.c

KCOLOURMAPO =	main_xt.o

$(KARMABINPATH)/kcolourmap:	$(KCOLOURMAPO) \
			$(KDEPLIB_KARMAGRAPHICS) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMAXT) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkcolourmap $(KCOLOURMAPO) $(CLIBS)
	install -s $(machine_dir)/tmpkcolourmap $(KARMABINPATH)/kcolourmap
	chmod u=rwx,go=x $(KARMABINPATH)/kcolourmap


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KCOLOURMAP)


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
