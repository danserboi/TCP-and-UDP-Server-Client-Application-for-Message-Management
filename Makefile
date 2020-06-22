# SERBOI FLOREA-DAN 325CB

CFLAGS = -Wall -g -Wextra

all: server subscriber

# Compileaza server.c glist.c
server: server.c glist.c

# Compileaza subscriber.c glist.c
subscriber: subscriber.c glist.c

.PHONY: clean run_server run_subscriber

# Ruleaza serverul cu: make run_server ARGS="<PORT_SERVER>"
# Alternativ, se poate da make si apoi ./server <PORT_SERVER>
run_server: ./server
	./server $(ARGS)

# Ruleaza subscriber-ul cu: make run_subscriber ARGS="<ID_CLIENT> <IP_SERVER> <PORT_SERVER>"
# Alternativ, se poate da make si apoi ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>
run_subscriber: ./subscriber
	./subscriber $(ARGS)

clean:
	rm -f server subscriber
