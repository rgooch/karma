include ../GNUmakefile.head


TARGETS =	$(KARMABINPATH)/kgen	$(KARMABINPATH)/kdump \
		$(KARMABINPATH)/raw_grey2karma \
		$(KARMABINPATH)/raw_planes2karma \
		$(KARMABINPATH)/karma2ps

all:	$(TARGETS)	clean


KGEN =	kgen.c
KGENO =	kgen.o

$(KARMABINPATH)/kgen:	$(KGENO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkgen $(KGENO) $(LIBS)
	install -s $(machine_dir)/tmpkgen $(KARMABINPATH)/kgen
	chmod u=rwx,go=x $(KARMABINPATH)/kgen


RAW_GREY2KARMA =	raw_grey2karma.c
RAW_GREY2KARMAO =	raw_grey2karma.o

$(KARMABINPATH)/raw_grey2karma:	$(RAW_GREY2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpraw_grey2karma $(RAW_GREY2KARMAO) $(LIBS)
	install -s $(machine_dir)/tmpraw_grey2karma $(KARMABINPATH)/raw_grey2karma
	chmod u=rwx,go=x $(KARMABINPATH)/raw_grey2karma


RAW_PLANES2KARMA =	raw_planes2karma.c
RAW_PLANES2KARMAO =	raw_planes2karma.o

$(KARMABINPATH)/raw_planes2karma:	$(RAW_PLANES2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpraw_planes2karma $(RAW_PLANES2KARMAO) $(LIBS)
	install -s $(machine_dir)/tmpraw_planes2karma $(KARMABINPATH)/raw_planes2karma
	chmod u=rwx,go=x $(KARMABINPATH)/raw_planes2karma


KDUMP =		kdump.c
KDUMPO =	kdump.o

$(KARMABINPATH)/kdump:	$(KDUMPO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkdump $(KDUMPO) $(LIBS)
	install -s $(machine_dir)/tmpkdump $(KARMABINPATH)/kdump
	chmod u=rwx,go=x $(KARMABINPATH)/kdump


KARMA2PS =	karma2ps.c
KARMA2PSO =	karma2ps.o

$(KARMABINPATH)/karma2ps:	$(KARMA2PSO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkarma2ps $(KARMA2PSO) $(LIBS)
	install -s $(machine_dir)/tmpkarma2ps $(KARMABINPATH)/karma2ps
	chmod u=rwx,go=x $(KARMABINPATH)/karma2ps


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c



clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(machine_dir)/depend
