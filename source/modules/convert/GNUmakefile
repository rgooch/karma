include ../GNUmakefile.head


TARGETS =	$(KARMABINPATH)/kgen \
		$(KARMABINPATH)/raw_grey2karma \
		$(KARMABINPATH)/raw_planes2karma \
		$(KARMABINPATH)/ppm2karma \
		$(KARMABINPATH)/fits2karma \
		$(KARMABINPATH)/miriad2karma \
		$(KARMABINPATH)/kdump \
		$(KARMABINPATH)/karma2ps \
		$(KARMABINPATH)/karma2ppm \
		$(KARMABINPATH)/karma2sunras

all:	$(TARGETS)	generic_clean


KGEN =	kgen.c
KGENO =	kgen.o

$(KARMABINPATH)/kgen:	$(KGENO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkgen $(KGENO) $(CLIBS)
	install -s $(machine_dir)/tmpkgen $(KARMABINPATH)/kgen
	chmod u=rwx,go=x $(KARMABINPATH)/kgen


RAW_GREY2KARMA =	raw_grey2karma.c
RAW_GREY2KARMAO =	raw_grey2karma.o

$(KARMABINPATH)/raw_grey2karma:	$(RAW_GREY2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpraw_grey2karma $(RAW_GREY2KARMAO) $(CLIBS)
	install -s $(machine_dir)/tmpraw_grey2karma $(KARMABINPATH)/raw_grey2karma
	chmod u=rwx,go=x $(KARMABINPATH)/raw_grey2karma


RAW_PLANES2KARMA =	raw_planes2karma.c
RAW_PLANES2KARMAO =	raw_planes2karma.o

$(KARMABINPATH)/raw_planes2karma:	$(RAW_PLANES2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpraw_planes2karma $(RAW_PLANES2KARMAO) $(CLIBS)
	install -s $(machine_dir)/tmpraw_planes2karma $(KARMABINPATH)/raw_planes2karma
	chmod u=rwx,go=x $(KARMABINPATH)/raw_planes2karma


PPM2KARMA =	ppm2karma.c
PPM2KARMAO =	ppm2karma.o

$(KARMABINPATH)/ppm2karma:	$(PPM2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir);$(LD) $(LDFLAGS) -o tmpppm2karma $(PPM2KARMAO) $(CLIBS)
	install -s $(machine_dir)/tmpppm2karma $(KARMABINPATH)/ppm2karma
	chmod u=rwx,go=x $(KARMABINPATH)/ppm2karma


FITS2KARMA =	fits2karma.c
FITS2KARMAO =	fits2karma.o

$(KARMABINPATH)/fits2karma:	$(FITS2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir);$(LD) $(LDFLAGS) -o tmpfits2karma $(FITS2KARMAO) $(CLIBS)
	install -s $(machine_dir)/tmpfits2karma $(KARMABINPATH)/fits2karma
	chmod u=rwx,go=x $(KARMABINPATH)/fits2karma


MIRIAD2KARMA =	miriad2karma.c
MIRIAD2KARMAO =	miriad2karma.o

$(KARMABINPATH)/miriad2karma:	$(MIRIAD2KARMAO) $(KDEPLIB_KARMA)
	cd $(machine_dir);$(LD) $(LDFLAGS) -o tmpmiriad2karma $(MIRIAD2KARMAO) $(CLIBS)
	install -s $(machine_dir)/tmpmiriad2karma $(KARMABINPATH)/miriad2karma
	chmod u=rwx,go=x $(KARMABINPATH)/miriad2karma


KDUMP =		kdump.c
KDUMPO =	kdump.o

$(KARMABINPATH)/kdump:	$(KDUMPO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkdump $(KDUMPO) $(CLIBS)
	install -s $(machine_dir)/tmpkdump $(KARMABINPATH)/kdump
	chmod u=rwx,go=x $(KARMABINPATH)/kdump


KARMA2PS =	karma2ps.c
KARMA2PSO =	karma2ps.o

$(KARMABINPATH)/karma2ps:	$(KARMA2PSO) $(KDEPLIB_KARMA)
	cd $(machine_dir);$(LD) $(LDFLAGS) -o tmpkarma2ps $(KARMA2PSO) $(CLIBS)
	install -s $(machine_dir)/tmpkarma2ps $(KARMABINPATH)/karma2ps
	chmod u=rwx,go=x $(KARMABINPATH)/karma2ps


KARMA2PPM =	karma2ppm.c
KARMA2PPMO =	karma2ppm.o

$(KARMABINPATH)/karma2ppm:	$(KARMA2PPMO) $(KDEPLIB_KARMA)
	cd $(machine_dir);$(LD) $(LDFLAGS) -o tmpkarma2ppm $(KARMA2PPMO) $(CLIBS)
	install -s $(machine_dir)/tmpkarma2ppm $(KARMABINPATH)/karma2ppm
	chmod u=rwx,go=x $(KARMABINPATH)/karma2ppm


KARMA2SUNRAS =	karma2sunras.c
KARMA2SUNRASO =	karma2sunras.o

$(KARMABINPATH)/karma2sunras:	$(KARMA2SUNRASO) $(KDEPLIB_KARMA)
	cd $(machine_dir);$(LD) $(LDFLAGS) -o tmpkarma2sunras $(KARMA2SUNRASO) $(CLIBS)
	install -s $(machine_dir)/tmpkarma2sunras $(KARMABINPATH)/karma2sunras
	chmod u=rwx,go=x $(KARMABINPATH)/karma2sunras


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include ../GNUmakefile.tail
include $(machine_dir)/depend
