CC=gcc
INC =  -I. -I.. -I../common/ -I../public/ -I../public/hft/ -I../tools -I../utils/ -I../hft/
CFLAGS=$(INC)

VPATH =../common ../hft ../tools ../public/hft

SRC= stn_getopt.c \
     fix_server_test.c\
	 hft_config.c\
	 fix_client_login.c\
	 fix_new_order.c\
	 fix_order_replace.c\
	 fix_order_cancel.c

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

ODIR=obj
LDIR=..

ifeq ($(PAX),Y)
	TARGETAPP = pax_hft_sample.exe
	LIBS=-lpthread -lrt -lnuma -L$(LDIR) -lpaxhft
else 
	TARGETAPP = fix_test_server.exe
	#LIBS=-Wl,-rpath,$(LDIR) -lstnhft -lpthread -lrt -lnuma
	LIBS= -L$(LDIR) -lstnhft -lpthread -lrt -lnuma	
endif

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

