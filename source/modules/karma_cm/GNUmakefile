ifeq ($(OS),Solaris)
MODULELIBS = -lnsl
endif

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head

ifeq ($(OS),SunOS)
ifdef VXHOME
vx_incswitch = -I$(VXHOME)/include
CC = cc -DOS_$(OS) -DMACHINE_$(MACHINE) -I$(KARMAINCLUDEPATH) $(KOPTIMISE)
LD = cc $(lpath)
endif
endif

TARGETS =	$(KARMABINPATH)/karma_cm_slave	$(KARMABINPATH)/cmshell

ifdef vx_incswitch
all:	$(TARGETS)	$(KARMABINPATH)/vxkarma_cm_slave	generic_clean
else
all:	$(TARGETS)	generic_clean
endif


KARMA_CM_SLAVE =	karma_cm_slave.c	fork.c	slave_setup.c
KARMA_CM_SLAVEO =	karma_cm_slave.o	fork.o	slave_setup.o

$(KARMABINPATH)/karma_cm_slave:	$(KARMA_CM_SLAVE) $(KARMA_CM_SLAVEO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkarma_cm_slave $(KARMA_CM_SLAVEO) $(CLIBS)
	install -s $(machine_dir)/tmpkarma_cm_slave $(KARMABINPATH)/karma_cm_slave
	chmod u=rwx,go=x $(KARMABINPATH)/karma_cm_slave


CMSHELL =	cmshell.c	fork.c
CMSHELLO =	cmshell.o	fork.o

$(KARMABINPATH)/cmshell:	$(CMSHELL) $(CMSHELLO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpcmshell $(CMSHELLO) $(CLIBS)
	install -s $(machine_dir)/tmpcmshell $(KARMABINPATH)/cmshell
	chmod u=rwx,go=x $(KARMABINPATH)/cmshell


VXKARMA_CM_SLAVE =	vxkarma_cm_slave.c	slave_setup.c
VXKARMA_CM_SLAVEO =	vxkarma_cm_slave.o	slave_setup.o

$(KARMABINPATH)/vxkarma_cm_slave:	$(VXKARMA_CM_SLAVE) $(VXKARMA_CM_SLAVEO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpvxkarma_cm_slave $(VXKARMA_CM_SLAVEO) $(CLIBS) -L$(VXHOME)/lib -lwm -ltask -lvx
	install -s $(machine_dir)/tmpvxkarma_cm_slave $(KARMABINPATH)/vxkarma_cm_slave
	chmod u=rwx,go=x $(KARMABINPATH)/vxkarma_cm_slave

$(machine_dir)/vxkarma_cm_slave.o:	vxkarma_cm_slave.c
	$(LD) -c $(CFLAGS) $(vx_incswitch) vxkarma_cm_slave.c -o $(machine_dir)/vxkarma_cm_slave.o


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) $(vx_incswitch) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
