CC = $(KARMA_CC)
CDEBUG = -g
# The  -O3  options causes problems, use  -O2  instead
COPTIMIZE = -O
#CSINGLE = -fd
CFLAGS = $(COPTIMIZE) -I$(KARMABASE)/include $(CSINGLE) -DARCH_$(MACHINE_ARCH)
FC = fc -fi
FFLAGS = $(COPTIMIZE)

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


TARGETS =	$(KARMABINPATH)/convert	$(KARMABINPATH)/histogram

all:	$(TARGETS)	clean


CONVERT =	convert.c
CONVERTO =	convert.o

$(KARMABINPATH)/convert:	$(CONVERTO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpconvert $(CONVERTO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpconvert $(KARMABINPATH)/convert
	chmod u=rwx,go=x $(KARMABINPATH)/convert


HISTOGRAM =	histogram.c
HISTOGRAMO =	histogram.o

$(KARMABINPATH)/histogram:	$(HISTOGRAMO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmphistogram $(HISTOGRAMO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmphistogram $(KARMABINPATH)/histogram
	chmod u=rwx,go=x $(KARMABINPATH)/histogram


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(MACHINE_ARCH)/depend *.c


clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(MACHINE_ARCH)/depend
