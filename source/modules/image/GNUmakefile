include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/kregrid

all:	$(TARGETS)	generic_clean


KREGRID  = kregrid.c
KREGRIDO = kregrid.o

$(KARMABINPATH)/kregrid:	$(KREGRIDO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkregrid $(KREGRIDO) $(CLIBS)
	install -s $(machine_dir)/tmpkregrid $(KARMABINPATH)/kregrid
	chmod u=rwx,go=x $(KARMABINPATH)/kregrid


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
