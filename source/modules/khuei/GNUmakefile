KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAINCLUDEPATH)/gmakedefs/modules.head


TARGETS =	$(KARMABINPATH)/khuei

all:	$(TARGETS)	generic_clean


KHUEI =		main_xt.c

KHUEIO =	main_xt.o

$(KARMABINPATH)/khuei:	$(KHUEIO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkhuei $(KHUEIO) $(CLIBS)
	\rm -f $(KARMABINPATH)/khuei
	install -s $(machine_dir)/tmpkhuei $(KARMABINPATH)/khuei
	chmod u=rwx,go=x $(KARMABINPATH)/khuei


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KHUEI)


include $(KARMAINCLUDEPATH)/gmakedefs/modules.tail
include $(machine_dir)/depend
