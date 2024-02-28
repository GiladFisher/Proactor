

all: server client

server: 
	gcc -o server server.c -pthread

client:
	gcc -o client client.c -pthread

proact_server:
	gcc -o proact_server proact_server.c -pthread

clean:
	rm server client proact_server