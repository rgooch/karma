KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/kslice_3d

all:	$(TARGETS)	generic_clean


KSLICE_3D =	main_xt.c

KSLICE_3DO =	main_xt.o

$(KARMABINPATH)/kslice_3d:	$(KSLICE_3DO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) $(KDEPLIB_KARMAXT) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkslice_3d $(KSLICE_3DO) $(CLIBS)
	install -s $(machine_dir)/tmpkslice_3d $(KARMABINPATH)/kslice_3d
	chmod u=rwx,go=x $(KARMABINPATH)/kslice_3d


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KSLICE_3D) 


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
