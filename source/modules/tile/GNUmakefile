include ../GNUmakefile.head


TARGETS =	$(KARMABINPATH)/tile1

all:	$(TARGETS)	clean


TILE1 =		tile1.c
TILE1O =	tile1.o

$(KARMABINPATH)/tile1:	$(TILE1O) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmptile1 $(TILE1O) $(LIBS)
	install -s $(machine_dir)/tmptile1 $(KARMABINPATH)/tile1
	chmod u=rwx,go=x $(KARMABINPATH)/tile1


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(machine_dir)/depend
