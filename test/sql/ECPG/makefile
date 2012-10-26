#
# Filename: makefile
#

INCLUDES   = -I/usr/include/pgsql

CFLAGS    += $(INCLUDES) -g

LDFLAGS   += -g

LDLIBS    += -lecpg -lpq

ECPGFLAGS += -c $(INCLUDES)

ECPG       = /usr/bin/ecpg

.SUFFIXES: .pgc

.pgc.c:
	$(ECPG) $(ECPGFLAGS) $?

ALL  = client1a client1b client2a client2b client2c

ALL += client3a client3b client3c client3d client3e client3f

ALL += client4.pgc

all: $(ALL)

clean:
	rm -f $(ALL) *~
