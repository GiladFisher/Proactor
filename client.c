#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 256

int sockfd;


// Send messages to server
void *send_message(void *arg) {
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        printf("Enter message: ");
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE - 1, stdin);

        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) {
            perror("ERROR writing to socket");
            break;
        }
    }

    pthread_exit(NULL);
}
// Receive messages from server
void *receive_message(void *arg) {
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n < 0) {
            perror("ERROR reading from socket");
            break;
        } else if (n == 0) {
            printf("Server closed connection\n");
            break;
        }

        printf("Server response: %s\n", buffer);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    pthread_t send_thread, receive_thread;
    const char *server_ip = "127.0.0.1";
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    // to reuse the address
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }

    // Initialize server address
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi("8080"));
    
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", argv[1]);
        exit(1);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting to server");
        exit(1);
    }

    // Create thread that will listen for user input and send messages to server
    if (pthread_create(&send_thread, NULL, send_message, NULL) != 0) { 
        perror("Thread creation failed");
        exit(1);
    }

    // Create thread for receiving messages
    if (pthread_create(&receive_thread, NULL, receive_message, NULL) != 0) {
        perror("Thread creation failed");
        exit(1);
    }

    // Wait for threads to finish
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    // Close socket
    close(sockfd);

    return 0;
}
