include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/tile1

all:	$(TARGETS)	generic_clean


TILE1 =		tile1.c
TILE1O =	tile1.o

$(KARMABINPATH)/tile1:	$(TILE1O) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmptile1 $(TILE1O) $(CLIBS)
	install -s $(machine_dir)/tmptile1 $(KARMABINPATH)/tile1
	chmod u=rwx,go=x $(KARMABINPATH)/tile1


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
