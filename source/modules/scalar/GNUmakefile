include ../GNUmakefile.head


TARGETS =	$(KARMABINPATH)/convert	$(KARMABINPATH)/histogram \
		$(KARMABINPATH)/kminmax

all:	$(TARGETS)	clean


CONVERT =	convert.c
CONVERTO =	convert.o

$(KARMABINPATH)/convert:	$(CONVERTO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpconvert $(CONVERTO) $(LIBS)
	install -s $(machine_dir)/tmpconvert $(KARMABINPATH)/convert
	chmod u=rwx,go=x $(KARMABINPATH)/convert


HISTOGRAM =	histogram.c
HISTOGRAMO =	histogram.o

$(KARMABINPATH)/histogram:	$(HISTOGRAMO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmphistogram $(HISTOGRAMO) $(LIBS)
	install -s $(machine_dir)/tmphistogram $(KARMABINPATH)/histogram
	chmod u=rwx,go=x $(KARMABINPATH)/histogram


KMINMAX =	kminmax.c
KMINMAXO =	kminmax.o

$(KARMABINPATH)/kminmax:	$(KMINMAXO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkminmax $(KMINMAXO) $(LIBS)
	install -s $(machine_dir)/tmpkminmax $(KARMABINPATH)/kminmax
	chmod u=rwx,go=x $(KARMABINPATH)/kminmax


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(machine_dir)/depend
