include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/convert	$(KARMABINPATH)/histogram \
		$(KARMABINPATH)/kminmax

all:	$(TARGETS)	generic_clean


CONVERT =	convert.c
CONVERTO =	convert.o

$(KARMABINPATH)/convert:	$(CONVERTO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpconvert $(CONVERTO) $(CLIBS)
	install -s $(machine_dir)/tmpconvert $(KARMABINPATH)/convert
	chmod u=rwx,go=x $(KARMABINPATH)/convert


HISTOGRAM =	histogram.c
HISTOGRAMO =	histogram.o

$(KARMABINPATH)/histogram:	$(HISTOGRAMO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmphistogram $(HISTOGRAMO) $(CLIBS)
	install -s $(machine_dir)/tmphistogram $(KARMABINPATH)/histogram
	chmod u=rwx,go=x $(KARMABINPATH)/histogram


KMINMAX =	kminmax.c
KMINMAXO =	kminmax.o

$(KARMABINPATH)/kminmax:	$(KMINMAXO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkminmax $(KMINMAXO) $(CLIBS)
	install -s $(machine_dir)/tmpkminmax $(KARMABINPATH)/kminmax
	chmod u=rwx,go=x $(KARMABINPATH)/kminmax


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
