run:
	gcc client.c -o client -Wall -g
	gcc server.c -o server -Wall -lpthread -g
	gcc backup_server.c -o backup_server -Wall -lrt -lpthread -g
	
clean:
	rm *.o client server backup_server pathToServerFifo server_logs client_logs 