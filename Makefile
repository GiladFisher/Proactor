

all: server client

server: 
	gcc -o server server.c -pthread

client:
	gcc -o client client.c -pthread

clean:
	rm server client