include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/kmkshell $(KARMABINPATH)/kfill_sphere \
		$(KARMABINPATH)/kmask3d

all:	$(TARGETS)	generic_clean


KMKSHELL  = kmkshell.c
KMKSHELLO = kmkshell.o

$(KARMABINPATH)/kmkshell:	$(KMKSHELLO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkmkshell $(KMKSHELLO) $(CLIBS)
	install -s $(machine_dir)/tmpkmkshell $(KARMABINPATH)/kmkshell
	chmod u=rwx,go=x $(KARMABINPATH)/kmkshell


KFILL_SPHERE  = kfill_sphere.c
KFILL_SPHEREO = kfill_sphere.o

$(KARMABINPATH)/kfill_sphere:	$(KFILL_SPHEREO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkfill_sphere $(KFILL_SPHEREO) $(CLIBS)
	install -s $(machine_dir)/tmpkfill_sphere $(KARMABINPATH)/kfill_sphere
	chmod u=rwx,go=x $(KARMABINPATH)/kfill_sphere


KMASK3D  = kmask3d.c
KMASK3DO = kmask3d.o

$(KARMABINPATH)/kmask3d:	$(KMASK3DO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkmask3d $(KMASK3DO) $(CLIBS)
	install -s $(machine_dir)/tmpkmask3d $(KARMABINPATH)/kmask3d
	chmod u=rwx,go=x $(KARMABINPATH)/kmask3d


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
