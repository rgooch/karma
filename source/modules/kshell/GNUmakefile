KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/kshell

all:	$(TARGETS)	generic_clean


KSHELL  = main_xt.c generic.c event.c globals.c

KSHELLO = main_xt.o generic.o event.o globals.o

$(KARMABINPATH)/kshell:	$(KSHELLO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkshell $(KSHELLO) $(CLIBS)
	\rm -f $(KARMABINPATH)/kshell
	install -s $(machine_dir)/tmpkshell $(KARMABINPATH)/kshell
	chmod u=rwx,go=x $(KARMABINPATH)/kshell


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KSHELL)


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
