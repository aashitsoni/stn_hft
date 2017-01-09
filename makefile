SDIR=src
CC=gcc

INC = -I./common/ -I./public/ -I./public/hft/ -I./hft/ -I./utils/ -I./tools/ -I. -I..

CFLAGS=$(INC)
LIBFLAGS = -shared -Wl, -,-soname,

VPATH = ./common ./hft ./utils ./tools/ ./public/hft \

SRC= stn_hft_mkt_data_core.c\
	 stn_hft_fix_op_core.c\
	 stn_numa_impl.c\
	 stn_hft_pair_strategy_core.c\
	 stn_hft_fix_msgs.c\
	 stn_hft_fix_order_db.c\
	 stn_hft_fix_handler.c\
	 console_log.c

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
# -msse4.2 -xSSE4.2

ifeq ($(PAX),Y)
	TARGETAPP = libpaxhft.a
else 
	TARGETAPP = libstnhft.so.1	
#	TARGETAPP_FULL = libstnhft.so.1.0.1
	TARGETAPP_FULL = libstnhft.so
endif

ODIR=obj
LDIR=libs
LD=icc

LIBS=-lpthread -lrt -lnuma

OBJS = $(patsubst %.c, $(ODIR)/%.o, $(SRC))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -fpic -g -Wall -o $@ $< $(CFLAGS)

$(TARGETAPP): $(OBJS)
	@echo making $(TARGETAPP) in $(TARGET) mode..
#	$(CC) $(LIBFLAGS) -o$(LDIR)/$(TARGETAPP) $(ODIR)/*.o
#	gcc -shared -Wl,-soname, $(TARGETAPP) -o $(LDIR)/$(TARGETAPP_FULL) $(ODIR)/*.o
	gcc -shared -o $(TARGETAPP_FULL) $(ODIR)/*.o

.PHONY: clean

clean:
	@echo cleaning up the object files
	rm -f $(ODIR)/*.o *~ $(LDIR)/$(TARGETAPP_FULL) $(IDIR)/*~ *.o
