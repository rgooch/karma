include $(KARMAROOT)/source/modules/GNUmakefile.head


TARGETS =	$(KARMABINPATH)/chlen	$(KARMABINPATH)/kfuncgen

all:	$(TARGETS)	generic_clean


CHLEN =		chlen.c
CHLENO =	chlen.o

$(KARMABINPATH)/chlen:	$(CHLENO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpchlen $(CHLENO) $(CLIBS)
	install -s $(machine_dir)/tmpchlen $(KARMABINPATH)/chlen
	chmod u=rwx,go=x $(KARMABINPATH)/chlen


KFUNCGEN =	kfuncgen.c
KFUNCGENO =	kfuncgen.o

$(KARMABINPATH)/kfuncgen:	$(KFUNCGENO) $(KDEPLIB_KARMA)
	cd $(machine_dir);$(LD) $(LDFLAGS) -o tmpkfuncgen $(KFUNCGENO) $(CLIBS)
	install -s $(machine_dir)/tmpkfuncgen $(KARMABINPATH)/kfuncgen
	chmod u=rwx,go=x $(KARMABINPATH)/kfuncgen


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAROOT)/source/modules/GNUmakefile.tail
include $(machine_dir)/depend
