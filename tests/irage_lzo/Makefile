
PROGRAM = irage_lzo
SOURCES = irage_test.c minilzo.c


default:
	@echo "Please choose one of the following targets:"
	@echo "    gcc:   gcc"


# Make sure that minilzo.h, lzoconf.h and lzodefs.h are in the
# current dircectory. Otherwise you may want to adjust CPPFLAGS.
##CPPFLAGS = -I../include/lzo -I.

#GCC_CFLAGS = -s -Wall -O3 -g -fomit-frame-pointer

GCC_CFLAGS = -s -Wall -O3


#
# gcc (generic)
#

gcc:
	gcc $(CPPFLAGS) $(GCC_CFLAGS) -o $(PROGRAM) $(SOURCES)

cc:
	cc $(CPPFLAGS) -o $(PROGRAM) $(SOURCES)

#/opt/intel/Compiler/11.1/069/bin/intel64/icc -s -Wall -O3 -o lzo_test irage_test.c minilzo.c

#
# other targets
#

clean:
	rm -f $(PROGRAM) $(PROGRAM).exe $(PROGRAM).map $(PROGRAM).tds
	rm -f *.err *.o *.obj

.PHONY: default clean

