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


TARGETS =	$(KARMABINPATH)/kmovie

all:	$(TARGETS)	clean


KMOVIE =	kmovie_main.c		kmovie_xview.c \
		kmovie_colour.c \
		kmovie_track.c		kmovie_event.c

KMOVIEO =	kmovie_main.o		kmovie_xview.o \
		kmovie_colour.o \
		kmovie_track.o		kmovie_event.o

$(KARMABINPATH)/kmovie:	$(KMOVIE) $(KMOVIEO) $(KDEPLIB_KARMAGRAPHICS) \
			$(KDEPLIB_KARMAXVIEW) \
			$(KDEPLIB_KARMAX11) $(KDEPLIB_KARMA)
	cd $(MACHINE_ARCH); $(KARMA_CC) $(LDFLAGS) -o tmpkmovie $(KMOVIEO) $(CLIBS)
	install -s $(MACHINE_ARCH)/tmpkmovie $(KARMABINPATH)/kmovie
	chmod u=rwx,go=x $(KARMABINPATH)/kmovie


depend:
	makedepend -I$(KARMAINCLUDEPATH) -I$(XINCLUDEPATH) -f$(MACHINE_ARCH)/depend $(KMOVIE) 



clean:
	rm -f *.BAK *.log *.sed .#* *~ *.bak */*.bak */tmp* .*defaults


remove:
	\rm -f $(TARGETS)

include $(MACHINE_ARCH)/depend
