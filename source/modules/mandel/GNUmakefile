include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/mandel

all:	$(TARGETS)	generic_clean


MANDEL =	mandel.c	compute_line.c
MANDELO =	mandel.o	compute_line.o

$(KARMABINPATH)/mandel:	$(MANDELO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpmandel $(MANDELO) $(CLIBS)
	install -s $(machine_dir)/tmpmandel $(KARMABINPATH)/mandel
	chmod u=rwx,go=x $(KARMABINPATH)/mandel


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
