CC = $(KARMA_CC)
CDEBUG = -g
# The  -O3  options causes problems, use  -O2  instead
COPTIMIZE = -O
#CSINGLE = -fd
CFLAGS = $(COPTIMIZE) -I$(KARMABASE)/include $(CSINGLE) $(CPIC) -DARCH_$(MACHINE_ARCH)
FC = fc -fi
FFLAGS = $(COPTIMIZE) $(CPIC)

LDFLAGS = $(COPTIMIZE)

vpath %.o
vpath %.o $(MACHINE_ARCH)

%.o : %.c
	$(CC) $(CFLAGS) -c $*.c -o $(MACHINE_ARCH)/$*.o

$(MACHINE_ARCH)/%.o : %.c
	$(CC) $(CFLAGS) -c $*.c -o $(MACHINE_ARCH)/$*.o

CLIBS =	$(LIBS)
FLIBS = $(LIBS)

LIBS =	-L$(KARMALIBPATH) -lkarma  -lm $(OS_LIBS)


TARGETS =	$(KARMABINPATH)/tx	$(KARMABINPATH)/rx \
		$(KARMABINPATH)/collect_struct $(KARMABINPATH)/collect_frames \
		$(KARMABINPATH)/conv_24to8	$(KARMABINPATH)/merge_planes

all:	$(TARGETS)	clean


TX =	tx.c
TXO =	tx.o

$(KARMABINPATH)/tx:	$(TXO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmptx $(TXO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmptx $(KARMABINPATH)/tx
	chmod u=rwx,go=x $(KARMABINPATH)/tx


RX =	rx.c
RXO =	rx.o

$(KARMABINPATH)/rx:	$(RXO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmprx $(RXO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmprx $(KARMABINPATH)/rx
	chmod u=rwx,go=x $(KARMABINPATH)/rx


COLLECT_STRUCT =	collect_struct.c
COLLECT_STRUCTO =	collect_struct.o

$(KARMABINPATH)/collect_struct:	$(COLLECT_STRUCTO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpcollect_struct $(COLLECT_STRUCTO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpcollect_struct $(KARMABINPATH)/collect_struct
	chmod u=rwx,go=x $(KARMABINPATH)/collect_struct


COLLECT_FRAMES =	collect_frames.c
COLLECT_FRAMESO =	collect_frames.o

$(KARMABINPATH)/collect_frames:	$(COLLECT_FRAMESO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpcollect_frames $(COLLECT_FRAMESO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpcollect_frames $(KARMABINPATH)/collect_frames
	chmod u=rwx,go=x $(KARMABINPATH)/collect_frames


CONV_24TO8 =	conv_24to8.c
CONV_24TO8O =	conv_24to8.o

$(KARMABINPATH)/conv_24to8:	$(CONV_24TO8O) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpconv_24to8 $(CONV_24TO8O) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpconv_24to8 $(KARMABINPATH)/conv_24to8
	chmod u=rwx,go=x $(KARMABINPATH)/conv_24to8


MERGE_PLANES =	merge_planes.c
MERGE_PLANESO =	merge_planes.o

$(KARMABINPATH)/merge_planes:	$(MERGE_PLANESO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpmerge_planes $(MERGE_PLANESO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpmerge_planes $(KARMABINPATH)/merge_planes
	chmod u=rwx,go=x $(KARMABINPATH)/merge_planes


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(MACHINE_ARCH)/depend *.c


clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(MACHINE_ARCH)/depend
