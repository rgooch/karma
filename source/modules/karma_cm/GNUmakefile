# THIS IS A HACK! I am forcing the use of  cc  rather than $(KARMA_CC) because
# if $(KARMA_CC) is  gcc  then  vxkarma_cm_slave  will not exit!
# I think this is because  gcc  screws around with the entry and exit procedure
CC = cc
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


TARGETS =	$(KARMABINPATH)/karma_cm_slave	$(KARMABINPATH)/cmshell

all:	$(TARGETS)	clean

vx:	$(TARGETS)	$(KARMABINPATH)/vxkarma_cm_slave	clean


KARMA_CM_SLAVE =	karma_cm_slave.c	fork.c	slave_setup.c
KARMA_CM_SLAVEO =	karma_cm_slave.o	fork.o	slave_setup.o

$(KARMABINPATH)/karma_cm_slave:	$(KARMA_CM_SLAVE) $(KARMA_CM_SLAVEO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpkarma_cm_slave $(KARMA_CM_SLAVEO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpkarma_cm_slave $(KARMABINPATH)/karma_cm_slave
	chmod u=rwx,go=x $(KARMABINPATH)/karma_cm_slave


CMSHELL =	cmshell.c	fork.c
CMSHELLO =	cmshell.o	fork.o

$(KARMABINPATH)/cmshell:	$(CMSHELL) $(CMSHELLO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(CC) $(LDFLAGS) -o tmpcmshell $(CMSHELLO) $(LIBS)
	install -s $(MACHINE_ARCH)/tmpcmshell $(KARMABINPATH)/cmshell
	chmod u=rwx,go=x $(KARMABINPATH)/cmshell


VXKARMA_CM_SLAVE =	vxkarma_cm_slave.c	slave_setup.c
VXKARMA_CM_SLAVEO =	vxkarma_cm_slave.o	slave_setup.o

$(KARMABINPATH)/vxkarma_cm_slave:	$(VXKARMA_CM_SLAVE) $(VXKARMA_CM_SLAVEO) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(CC) $(LDFLAGS) -o tmpvxkarma_cm_slave $(VXKARMA_CM_SLAVEO) $(LIBS) -L$(VXHOME)/lib -ltask -lvx
	install -s $(MACHINE_ARCH)/tmpvxkarma_cm_slave $(KARMABINPATH)/vxkarma_cm_slave
	chmod u=rwx,go=x $(KARMABINPATH)/vxkarma_cm_slave

$(MACHINE_ARCH)/vxkarma_cm_slave.o:	vxkarma_cm_slave.c
	$(CC) -c $(CFLAGS) -I$(VXHOME)/include vxkarma_cm_slave.c -o $(MACHINE_ARCH)/vxkarma_cm_slave.o


depend:
	makedepend -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -I$(VXHOME)/include -f$(MACHINE_ARCH)/depend *.c


clean:
	rm -f *.BAK *.log *.sed date.h
	rm -f .#* *~ *.bak */*.bak
	rm -f */tmp*
	rm -f *.o
	rm -f .*defaults
	-chmod u+rw,go-w * 2> /dev/null


remove:
	\rm -f $(TARGETS)

include $(MACHINE_ARCH)/depend
