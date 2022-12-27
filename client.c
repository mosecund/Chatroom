/*
 * ==========================================================================================
 *
 *       Filename:  client.c
 *
 *    Description:  Code côté client qui gère l'envoie et la réception des messages via 
 *                  le serveur en utilisant les threads noyaux.
 *
 *        Created:  19/12/2021 
 *
 *        Authors:  Secundar Ismael && Pikop Dave
 *         Projet:  CHATROOM
 *
 * ==========================================================================================
 */

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "common.h" 

#define BUFFERSIZE 1024

int sockfd = 0;
char pseudo[32];


void parse_args(int args_nber, char **args){
    if (args_nber < 4){
        printf("Usage: ./client <pseudo> <ip_serveur> <port>\n");
        exit(-1);
    }
    else if (is_integer(args[3]) == 0){
        printf("%s n'est pas un port\n", args[3]); 
        exit(-1);  
    }  
}

struct buffer {
    size_t msg_lenght;
    time_t timestamp;
    char *message;
};

void start_of_msg_char() {
    printf("%s", "> ");
    fflush(stdout);    //oblige le système à vider le tampon associé au flux de sortie spécifié
}

/*remplace le retour à la ligne par le caractere de fin de ligne*/
void clean_up_msg(char *str, int str_size) {
    int i;
    for (i = 0; i < str_size; i++) { 
        if (str[i] == '\n') {
            str[i] = '\0';
            break;
        }
    }
}


/*print message sent by server*/
void recv_msg_handler() {
    char message[BUFFERSIZE] = {};
    while (1) {
        start_of_msg_char();
        int receive = checked(recv(sockfd, message, BUFFERSIZE, 0));
        if (receive > 0) {
            printf("%s\n", message);  
        }  else {                             // cas où le client reçoit un signal d'arrêt du serveur
            printf("fermeture en cours...\n");
            exit(0);
        }
    }
}


int main(int argc, char **argv) {
    // parsing des arguments
    parse_args(argc, argv);
    strcpy(pseudo, argv[1]);  

    char ip_server[strlen(argv[2])];         
    strcpy(ip_server, argv[2]);

    char port_[strlen(argv[3])];            
    strcpy(port_, argv[3]);

    long int port = strtol(port_, NULL, 10);    // convertit le port en long int

    clean_up_msg(pseudo, strlen(pseudo));

    /* Socket settings */
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_server);
    server_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        printf("ERROR: connect\n");
        return EXIT_FAILURE;
    }

    checked(send(sockfd, pseudo, 32, 0));   // send pseudo to server

    printf("<<<< WELCOME %s ON CHATROOM >>>>\n", pseudo);   

    /* message reception handler */
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0) {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    /* message sending handler */
    char *message = malloc(BUFFERSIZE + 1);
    while (fgets(message, BUFFERSIZE + 1, stdin)) {
        char *send_msg = malloc(BUFFERSIZE + 1);
        clean_up_msg(message, BUFFERSIZE + 1);

        // construction of message to send
        struct buffer buf;
        buf.message = strdup(message);               // duplique le char pointer pour eviter une segmentation default
        buf.msg_lenght = strlen(buf.message);           
        buf.timestamp = time(NULL);
        
        // conversion timestamp au format HHMMSS ou heures:minutes:secondes
        struct tm *tmp;
        char timestamp_converted[BUFFERSIZE];
        time( &buf.timestamp );
        tmp = localtime( &buf.timestamp );
        strftime(timestamp_converted,BUFFERSIZE,"%X", tmp);

        // send message to server
        sprintf(send_msg, "%s: %ld %s %s", pseudo, buf.msg_lenght, timestamp_converted, buf.message);  // stocke le msg "%s: %s\n" dans buffer(le tampon)
        checked(send(sockfd, send_msg, strlen(send_msg), 0));

        free(buf.message);
        free(send_msg);
    }
    free(message);
    close(sockfd);

    return EXIT_SUCCESS;
}