all:
	remove_oldlibs
	for i in karma* ;\
	do \
		(echo "Making in $$i"; cd $$i; gmake); \
	done

depend:
	for i in karma* ;\
	do \
		(echo "Depending in $$i"; cd $$i; gmake depend); \
	done

distclean:
	rm -f */*.o libkarma*.*
