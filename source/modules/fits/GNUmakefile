include ../GNUmakefile.head


TARGETS =	$(KARMABINPATH)/fits2karma

all:	$(TARGETS)	clean


FITS2KARMA =	fits2karma.c
FITS2KARMAO =	fits2karma.o

$(KARMABINPATH)/fits2karma:	$(FITS2KARMA) $(FITS2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpfits2karma $(FITS2KARMAO) $(LIBS)
	install -s $(machine_dir)/tmpfits2karma $(KARMABINPATH)/fits2karma
	chmod u=rwx,go=x $(KARMABINPATH)/fits2karma


depend:
	makedepend -I$(KARMAINCLUDEPATH) -DOS_$(OS) -DMACHINE_$(MACHINE) -f$(machine_dir)/depend *.c



clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(machine_dir)/depend
