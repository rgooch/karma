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


TARGETS =	$(KARMABINPATH)/chlen

all:	$(TARGETS)	clean


CHLEN =		chlen.c
CHLENO =	chlen.o

$(KARMABINPATH)/chlen:	$(CHLENO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpchlen $(CHLENO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpchlen $(KARMABINPATH)/chlen
	chmod u=rwx,go=x $(KARMABINPATH)/chlen


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(MACHINE_ARCH)/depend *.c



clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(MACHINE_ARCH)/depend
