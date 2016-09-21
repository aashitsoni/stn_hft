CC=gcc

INC = -I../common/ -I../public/ -I../public/hft/ -I../tools -I../utils/ -I../hft/ -I. -I..

CFLAGS=$(INC)

VPATH =../common ../hft ../tools ../public/hft

SRC= stn_getopt.c \
     main.hft.c

ifeq ($(RLDB),Y)
	DEBUG_FLAG = -ggdb3
	TARGET=RLDB
else ifeq ($(DB),Y)
	DEBUG_FLAG = -ggdb3 -DDEBUG_SYMBOLS=1
	TARGET=DB
else
	DEBUG_FLAG = -DDEBUG_SYMBOLS=0 -O3
	TARGET=REL
endif

CFLAGS:=$(DEBUG_FLAG) $(CFLAGS)

#-msse4.2 -xSSE4.2

ifeq ($(PAX),Y)
	TARGETAPP = pax_hft_sample.exe
	LIBS=-lpthread -lrt -lnuma -L$(LDIR) -lpaxhft
else 
	TARGETAPP = stn_hft_sample.exe
	LIBS=-lpthread -lrt -lnuma -L$(LDIR) -lstnhft
endif


ODIR=../obj
LDIR=../libs

#LD=icc


OBJS = $(patsubst %.c, $(ODIR)/%.o, $(SRC))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGETAPP): $(OBJS)
	@echo making $(TARGETAPP) in $(TARGET) mode..
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

.PHONY: clean

clean:
	@echo cleaning up the object files
	rm -f $(ODIR)/*.o *~ $(IDIR)/*~ *.o *.exe
