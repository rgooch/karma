CC = $(KARMA_CC)
# The  -O3  options causes problems, use  -O2  instead
COPTIMISE = -O
#CSINGLE = -fd
CFLAGS = $(COPTIMISE) -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) $(CSINGLE) -DARCH_$(MACHINE_ARCH) -DX11
FC = fc -fi
FFLAGS = $(COPTIMISE)

LDFLAGS = $(COPTIMISE)

vpath %.o
vpath %.o $(MACHINE_ARCH)

%.o : %.c
	$(CC) $(CFLAGS) -c $*.c -o $(MACHINE_ARCH)/$*.o

$(MACHINE_ARCH)/%.o : %.c
	$(CC) $(CFLAGS) -c $*.c -o $(MACHINE_ARCH)/$*.o

CLIBS =	$(LIBS) -lm
FLIBS = $(LIBS) -lc

LIBS =	-L$(KARMALIBPATH) -L$(XLIBPATH) \
	-lkarmagraphics -lkarmaxview -lkarmaX11 -lkarma \
	-lxview -lolgx -lXext -lX11 \
	-ltermcap $(OS_LIBS)


TARGETS =	$(KARMABINPATH)/kview_2d

all:	$(TARGETS)	clean


KVIEW_2D =	kview_2d_main.c		kview_2d_xview.c \
		kview_2d_colour.c \
		kview_2d_track.c		kview_2d_event.c

KVIEW_2DO =	kview_2d_main.o		kview_2d_xview.o \
		kview_2d_colour.o \
		kview_2d_track.o		kview_2d_event.o

$(KARMABINPATH)/kview_2d:	$(KVIEW_2DO) \
			$(KDEPLIB_KARMAGRAPHICS) $(KDEPLIB_KARMAXVIEW) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpkview_2d $(KVIEW_2DO) $(CLIBS)
	install -s $(MACHINE_ARCH)/tmpkview_2d $(KARMABINPATH)/kview_2d
	chmod u=rwx,go=x $(KARMABINPATH)/kview_2d


depend:
	makedepend -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(MACHINE_ARCH)/depend $(KVIEW_2D)



clean:
	rm -f *.BAK *.log *.sed date.h .#* *~ *.bak */*.bak */tmp* .*defaults


remove:
	\rm -f $(TARGETS)

include $(MACHINE_ARCH)/depend
