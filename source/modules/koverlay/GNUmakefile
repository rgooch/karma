KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/koverlay

all:	$(TARGETS)	generic_clean


KOVERLAY =	main_xt.c generic.c globals.c contour_xt.c

KOVERLAYO =	main_xt.o generic.o globals.o contour_xt.o

$(KARMABINPATH)/koverlay:	$(KOVERLAYO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkoverlay $(KOVERLAYO) $(CLIBS)
	\rm -f $(KARMABINPATH)/koverlay
	install -s $(machine_dir)/tmpkoverlay $(KARMABINPATH)/koverlay
	chmod u=rwx,go=x $(KARMABINPATH)/koverlay


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KOVERLAY)


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
