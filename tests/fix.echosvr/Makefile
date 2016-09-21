CC=gcc

INC = -I../common/ -I../public/ -I../public/hft/ -I../tools -I../utils/ -I../hft/ -I. -I..

CFLAGS=$(INC)

VPATH =../common ../hft ../tools ../public/hft

SRC= fix.echosvr.c \
     helper.c

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

TARGETAPP = fix.echosvr

LDIR=../libs

#LD=icc

LIBS=-lpthread -lrt  -L$(LDIR) 
OBJS = $(patsubst %.c, %.o, $(SRC))

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGETAPP): $(OBJS)
	@echo making $(TARGETAPP) in $(TARGET) mode..
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

.PHONY: clean

clean:
	@echo cleaning up the object files
	rm -f *.o *~ $(IDIR)/*~ $(TARGETAPP)




