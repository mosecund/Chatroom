/*
 * ==========================================================================================
 *
 *       Filename:  server.c
 *
 *    Description:  Code côté serveur qui gère plusieurs connexions socket avec select et 
 *                  fd_set sur linux, gère la réception et l’envoie des messages 
 *                  à tous les clients connectés.
 *
 *        Created:  19/12/2021 
 *
 *        Authors:  Secundar Ismael && Pikop Dave
 *         Projet:  CHATROOM
 *
 * ==========================================================================================
 */


#include <netinet/in.h>
#include <sys/times.h>  // FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // strlen
#include <unistd.h>   // close
#include <signal.h>   // SIGINT
#include "common.h"

#define BUFFERSIZE 1024

/*identifiants d'un client connecté*/
struct user {
    int socket;
    char pseudo[BUFFERSIZE];
};

/*gestionnaire du signal envoyé par le serveur*/
void sigintHandler(int sig_received){
    if (sig_received == SIGINT){
		printf("    Fermeture du chat...\n");
		exit(0);
	}
}

/*vérifie le nombre de paramètres et leurs types*/
void parse_args(int args_nber, char **args){
    if (args_nber < 2){
        printf("Usage: ./serveur <port>\n");
        exit(-1);
    }
    else if (is_integer(args[1]) == 0){
        printf("%s n'est pas un port\n", args[1]); 
        exit(-1);  
    }  
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    /*setup server*/
    int sockfd_server, sockfd_client, nclients = 0, opt = 1;          
    fd_set readfds;               // ensemble des fds en lecture qui seront surveillés par select
    struct sockaddr_in addr_server, addr_client;   // structure d'adresse qui contiendra les params reseaux du recepteur et de l'expediteur

    socklen_t sin_size = sizeof(struct sockaddr_in);

    sockfd_server = checked(socket(AF_INET, SOCK_STREAM, 0));
    checked(setsockopt(sockfd_server, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)));

    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(atoi(argv[1]));
    addr_server.sin_addr.s_addr = INADDR_ANY;

    checked(bind(sockfd_server, (struct sockaddr *) &addr_server, sizeof(addr_server)));
    checked(listen(sockfd_server, 10));

    struct user clients[FD_SETSIZE];          // table de tous les clients connectés de type user

    printf("Attente de connexion...\n");

    // ajoute le master socket dans la liste des clients
    clients[0].socket = sockfd_server;
    strcpy(clients[0].pseudo, "server");
    nclients++;       
    char buf_pseudo[BUFFERSIZE];               

    // ferme le serveur et provoque la fermeture de tous les clients
    signal(SIGINT, sigintHandler);
    while (1) {
        FD_ZERO(&readfds);                                        
        int max_fd = 0;
        for (int j = 0; j < nclients; j++) {
            // add all sockets in fd_set
            if (clients[j].socket != 0)    
                FD_SET(clients[j].socket, &readfds);  
            // getting max socket client for select
            if (max_fd < clients[j].socket)                 
                max_fd = clients[j].socket;
        }

        /*wait for an activity on one of the sockets, timeout is NULL*/
        checked(select(max_fd + 1, &readfds, NULL, NULL, NULL));
        if (FD_ISSET(sockfd_server, &readfds)) {  
            // Si c'est le master socket qui a des donnees, c'est une nouvelle connexion.                          
            sockfd_client = checked(accept(sockfd_server, (struct sockaddr *) &addr_client, &sin_size));
            if (read(sockfd_client, &buf_pseudo, BUFFERSIZE)){
                printf("Connexion etablie avec %s fd : %d\n", buf_pseudo, sockfd_client);
            }
            nclients++;                                                                    
            clients[nclients - 1].socket = sockfd_client;
            strcpy(clients[nclients - 1].pseudo, buf_pseudo);
        }
        else{
            // Sinon, c'est un message d'un client
          for (int i = 1; i < nclients; i++) {                                     
                if (FD_ISSET(clients[i].socket, &readfds)) {               // si un socket du tableau est dans readfds, alors qqch a été envoyé au serveur par un client
                    char *buf[BUFFERSIZE];               
                    size_t nbytes = read(clients[i].socket, (void*)&buf, BUFFERSIZE);
                    if (nbytes > 0){    
                        // envoie le message à tous les clients connectés 
                        for (int k = 1; k < nclients; k++) {                                                    
                            if (write(clients[k].socket, (void*)&buf, BUFFERSIZE) < 0)
                                perror("Erreur lors de l'appel a send -> ");
                        }
                    }else{       // en cas d'EOF sur le stdin
                        printf("Host disconnected from %s fd : %d\n" , clients[i].pseudo, clients[i].socket);
                        char *name = clients[i].pseudo;    
                        close( clients[i].socket );
                        clients[i].socket = clients[nclients - 1].socket;
                        // On deplace le dernier socket a la place de libre pour ne pas faire de trou.
                        strcpy(clients[i].pseudo, clients[nclients - 1].pseudo);
                        strcpy(clients[i].pseudo, clients[nclients - 1].pseudo);
                        nclients--;
                    }
                    memset(buf, '\0', BUFFERSIZE);    // réinitialise le buffer
                }   
            }
        }
    }
}