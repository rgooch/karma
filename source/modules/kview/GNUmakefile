KARMALIBS = -lkarmawidgets -lkarmagraphics -lkarmaXt -lkarmaX11 -lkarma
XLIBS = -lXaw -lXmu -lXt -lXext -lX11

include $(KARMAROOT)/source/modules/GNUmakefile.head


TARGETS =	$(KARMABINPATH)/kview

all:	$(TARGETS)	generic_clean


KVIEW =		main_xt.c generic.c

KVIEWO =	main_xt.o generic.o

$(KARMABINPATH)/kview:	$(KVIEWO) $(KDEPLIB_KARMAWIDGETS) \
			$(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkview $(KVIEWO) $(CLIBS)
	\rm -f $(KARMABINPATH)/kview
	install -s $(machine_dir)/tmpkview $(KARMABINPATH)/kview
	chmod u=rwx,go=x $(KARMABINPATH)/kview


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(machine_dir)/depend $(KVIEW)


include $(KARMAROOT)/source/modules/GNUmakefile.tail
include $(machine_dir)/depend
