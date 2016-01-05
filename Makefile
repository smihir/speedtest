CC = gcc
CSCOPE = cscope
CFLAGS += -Wall -Werror
LDFLAGS += -lpthread

SERVER-OBJS := server.o common.o\

CLIENT-OBJS := client.o common.o \

ifeq ($(DEBUG), y)
 CFLAGS += -g -DDEBUG
endif

.PHONY: all
all: server client

server: server.o common.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(SERVER-OBJS) -o $@

client: client.o common.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLIENT-OBJS) -o $@

%.o: %.c *.h
	$(CC) $(CFLAGS) -o $@ -c $<

cscope:
	$(CSCOPE) -bqR

.PHONY: clean
clean:
	rm -rf *.o server client
