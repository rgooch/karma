KARMALIBS = -lkarmagraphics -lkarmaX11 -lkarma
XLIBS = -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/send-overlays $(KARMABINPATH)/send-vectors \
		$(KARMABINPATH)/send-contours

all:	$(TARGETS)	generic_clean


SEND =	send-overlays.c

SENDO =	send-overlays.o

$(KARMABINPATH)/send-overlays:	$(SENDO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpsend-overlays $(SENDO) $(CLIBS)
	\rm -f $(KARMABINPATH)/send-overlays
	install -s $(machine_dir)/tmpsend-overlays $(KARMABINPATH)/send-overlays
	chmod u=rwx,go=x $(KARMABINPATH)/send-overlays


SENDVEC  = send-vectors.c

SENDVECO = send-vectors.o

$(KARMABINPATH)/send-vectors:	$(SENDVECO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpsend-vectors $(SENDVECO) $(CLIBS)
	\rm -f $(KARMABINPATH)/send-vectors
	install -s $(machine_dir)/tmpsend-vectors $(KARMABINPATH)/send-vectors
	chmod u=rwx,go=x $(KARMABINPATH)/send-vectors


SENDCONT  = send-contours.c

SENDCONTO = send-contours.o

$(KARMABINPATH)/send-contours:	$(SENDCONTO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpsend-contours $(SENDCONTO) $(CLIBS)
	\rm -f $(KARMABINPATH)/send-contours
	install -s $(machine_dir)/tmpsend-contours $(KARMABINPATH)/send-contours
	chmod u=rwx,go=x $(KARMABINPATH)/send-contours


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
