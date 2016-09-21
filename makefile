SDIR=src
CC=gcc


INC = -I./common/ -I./public/ \
      -I./public/hft/ -I./hft/ -I./utils/ -I./tools/ -I. -I..
      


CFLAGS=$(INC)

VPATH = ./common ./hft ./utils ./tools/ ./public/hft \

SRC= stn_hft_mkt_data_core.c stn_hft_fix_op_core.c stn_numa_impl.c  stn_hft_pair_strategy_core.c

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
	TARGETAPP = libstnhft.a	
endif

ODIR=obj
LDIR=libs
LD=icc

LIBS=-lpthread -lrt -lnuma

OBJS = $(patsubst %.c, $(ODIR)/%.o, $(SRC))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGETAPP): $(OBJS)
	@echo making $(TARGETAPP) in $(TARGET) mode..
	ar -cvq $(LDIR)/$(TARGETAPP) $(ODIR)/*.o
	

clean:
	@echo cleaning up the object files
	rm -f $(ODIR)/*.o *~ $(LDIR)/*.a $(IDIR)/*~ *.o
