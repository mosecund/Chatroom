all: client serveur

client: client.c 
	gcc -Wall -o client client.c -lpthread

server: serveur.c 
	gcc -Wall -o serveur serveur.c 
