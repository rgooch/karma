include $(KARMAROOT)/source/modules/GNUmakefile.head


TARGETS =	$(KARMABINPATH)/tx	$(KARMABINPATH)/rx \
		$(KARMABINPATH)/collect_struct $(KARMABINPATH)/collect_frames \
		$(KARMABINPATH)/conv_24to8	$(KARMABINPATH)/merge_planes \
		$(KARMABINPATH)/kftpd		$(KARMABINPATH)/kftp \
		$(KARMABINPATH)/tcplog		$(KARMABINPATH)/knoise

all:	$(TARGETS)	generic_clean


TX =	tx.c
TXO =	tx.o

$(KARMABINPATH)/tx:	$(TXO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmptx $(TXO) $(CLIBS)
	install -s $(machine_dir)/tmptx $(KARMABINPATH)/tx
	chmod u=rwx,go=x $(KARMABINPATH)/tx


RX =	rx.c
RXO =	rx.o

$(KARMABINPATH)/rx:	$(RXO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmprx $(RXO) $(CLIBS)
	install -s $(machine_dir)/tmprx $(KARMABINPATH)/rx
	chmod u=rwx,go=x $(KARMABINPATH)/rx


COLLECT_STRUCT =	collect_struct.c
COLLECT_STRUCTO =	collect_struct.o

$(KARMABINPATH)/collect_struct:	$(COLLECT_STRUCTO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpcollect_struct $(COLLECT_STRUCTO) $(CLIBS)
	install -s $(machine_dir)/tmpcollect_struct $(KARMABINPATH)/collect_struct
	chmod u=rwx,go=x $(KARMABINPATH)/collect_struct


COLLECT_FRAMES =	collect_frames.c
COLLECT_FRAMESO =	collect_frames.o

$(KARMABINPATH)/collect_frames:	$(COLLECT_FRAMESO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpcollect_frames $(COLLECT_FRAMESO) $(CLIBS)
	install -s $(machine_dir)/tmpcollect_frames $(KARMABINPATH)/collect_frames
	chmod u=rwx,go=x $(KARMABINPATH)/collect_frames


CONV_24TO8 =	conv_24to8.c
CONV_24TO8O =	conv_24to8.o

$(KARMABINPATH)/conv_24to8:	$(CONV_24TO8O) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpconv_24to8 $(CONV_24TO8O) $(CLIBS)
	install -s $(machine_dir)/tmpconv_24to8 $(KARMABINPATH)/conv_24to8
	chmod u=rwx,go=x $(KARMABINPATH)/conv_24to8


MERGE_PLANES =	merge_planes.c
MERGE_PLANESO =	merge_planes.o

$(KARMABINPATH)/merge_planes:	$(MERGE_PLANESO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpmerge_planes $(MERGE_PLANESO) $(CLIBS)
	install -s $(machine_dir)/tmpmerge_planes $(KARMABINPATH)/merge_planes
	chmod u=rwx,go=x $(KARMABINPATH)/merge_planes


KFTPD =	kftpd.c
KFTPDO =	kftpd.o

$(KARMABINPATH)/kftpd:	$(KFTPDO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkftpd $(KFTPDO) $(CLIBS)
	install -s $(machine_dir)/tmpkftpd $(KARMABINPATH)/kftpd
	chmod u=rwx,go=x $(KARMABINPATH)/kftpd


KFTP =	kftp.c
KFTPO =	kftp.o

$(KARMABINPATH)/kftp:	$(KFTPO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpkftp $(KFTPO) $(CLIBS)
	install -s $(machine_dir)/tmpkftp $(KARMABINPATH)/kftp
	chmod u=rwx,go=x $(KARMABINPATH)/kftp


TCPLOG =	tcplog.c
TCPLOGO =	tcplog.o

$(KARMABINPATH)/tcplog:	$(TCPLOGO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmptcplog $(TCPLOGO) $(CLIBS)
	install -s $(machine_dir)/tmptcplog $(KARMABINPATH)/tcplog
	chmod u=rwx,go=x $(KARMABINPATH)/tcplog


KNOISE =	knoise.c
KNOISEO =	knoise.o

$(KARMABINPATH)/knoise:	$(KNOISEO) $(KDEPLIB_KARMA)
	cd $(machine_dir); $(LD) $(LDFLAGS) -o tmpknoise $(KNOISEO) $(CLIBS)
	install -s $(machine_dir)/tmpknoise $(KARMABINPATH)/knoise
	chmod u=rwx,go=x $(KARMABINPATH)/knoise


depend:
	makedepend -DMAKEDEPEND -D__$(MACHINE)__ -I$(KARMAINCLUDEPATH) -f$(machine_dir)/depend *.c


include $(KARMAROOT)/source/modules/GNUmakefile.tail
include $(machine_dir)/depend
