ifdef PARALLEL
POPT = -j $(PARALLEL)
endif

all:
	for i in karma* ;\
	do \
		(echo "Making in $$i"; cd $$i; gmake $(POPT)); \
	done
	-remove_oldlibs

depend:
	for i in karma* ;\
	do \
		(echo "Depending in $$i"; cd $$i; gmake depend); \
	done

distclean:
	rm -f */*.o libkarma*.*
