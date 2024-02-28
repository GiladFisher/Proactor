#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void *handle_client(void *arg)
{
    int newsockfd = *((int *) arg);
    char buffer[256];
    int n;

    while (1) {
        // Clear buffer and read message from client
        bzero(buffer, sizeof(buffer));
        n = read(newsockfd, buffer, sizeof(buffer) - 1);
        if (n < 0) {
            error("ERROR reading from socket");
        } else if (n == 0) {
            // Client closed the connection
            break;
        }

        // Print received message from client
        printf("Received message from client (socket descriptor %d): %s\n", newsockfd, buffer);

        // Send response back to client
        n = write(newsockfd, "OK", 2);
        if (n < 0) {
            error("ERROR writing to socket");
        }
    }

    // Close connection and exit thread
    close(newsockfd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t thread_id;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 8080;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");

    listen(sockfd, 5);

    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");

        if (pthread_create(&thread_id, NULL, handle_client, (void *) &newsockfd) != 0) { // Create a new thread for each client
            perror("Thread creation failed");
            close(newsockfd);
        }
    }

    close(sockfd);
    return 0; 
}
