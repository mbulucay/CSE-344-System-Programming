
TAGS=-lrt -lpthread -pthread -lm -Wall -g
SRVOBJ=server.o queue.o
CFILES=structs.c lib.c


run:
	gcc $(TAGS) servant.c $(CFILES) -o myservant
	gcc $(TAGS) server.c queue.c $(CFILES) -o myserver
	gcc $(TAGS) client.c $(CFILES) -o myclient

clean:
	rm myclient myservant myserver


server:
	gcc $(TAGS) server.c queue.c $(CFILES) -o myserver

servant:
	gcc $(TAGS) servant.c $(CFILES) -o myservant

client:
	gcc $(TAGS) client.c $(CFILES) -o myclient

