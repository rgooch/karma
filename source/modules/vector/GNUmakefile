include ../GNUmakefile.head


TARGETS =	$(KARMABINPATH)/chlen

all:	$(TARGETS)	clean


CHLEN =		chlen.c
CHLENO =	chlen.o

$(KARMABINPATH)/chlen:	$(CHLENO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpchlen $(CHLENO) $(LIBS)
	install -s $(machine_dir)/tmpchlen $(KARMABINPATH)/chlen
	chmod u=rwx,go=x $(KARMABINPATH)/chlen


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c



clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(machine_dir)/depend
