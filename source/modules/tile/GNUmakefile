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


TARGETS =	$(KARMABINPATH)/tile1

all:	$(TARGETS)	clean


TILE1 =		tile1.c
TILE1O =	tile1.o

$(KARMABINPATH)/tile1:	$(TILE1O) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmptile1 $(TILE1O) $(LIBS)
	install -s $(MACHINE_ARCH)/tmptile1 $(KARMABINPATH)/tile1
	chmod u=rwx,go=x $(KARMABINPATH)/tile1


depend:
	makedepend -I$(KARMAINCLUDEPATH) -f$(MACHINE_ARCH)/depend *.c


clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(MACHINE_ARCH)/depend
