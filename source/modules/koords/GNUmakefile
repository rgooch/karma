KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/koords

all:	$(TARGETS)	generic_clean


KOORDS  = main_xt.c generic.c event.c globals.c compute.c solve_im.c mat.c

KOORDSO = main_xt.o generic.o event.o globals.o compute.o solve_im.o mat.o

$(KARMABINPATH)/koords:	$(KOORDSO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkoords $(KOORDSO) $(CLIBS)
	\rm -f $(KARMABINPATH)/koords
	install -s $(machine_dir)/tmpkoords $(KARMABINPATH)/koords
	chmod u=rwx,go=x $(KARMABINPATH)/koords


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KOORDS)


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
