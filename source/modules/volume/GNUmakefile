include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/kmkshell

all:	$(TARGETS)	generic_clean


KMKSHELL  = kmkshell.c
KMKSHELLO = kmkshell.o

$(KARMABINPATH)/kmkshell:	$(KMKSHELLO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkmkshell $(KMKSHELLO) $(CLIBS)
	install -s $(machine_dir)/tmpkmkshell $(KARMABINPATH)/kmkshell
	chmod u=rwx,go=x $(KARMABINPATH)/kmkshell


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
