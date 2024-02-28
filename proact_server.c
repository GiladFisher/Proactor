#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 256

// Structure for client context
typedef struct {
    int fd; // File descriptor for the socket
    char buffer[BUFFER_SIZE]; // Buffer for read/write operations
    size_t buffer_len; // Length of data in the buffer
} ClientContext;

// Function to set socket to non-blocking mode
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

// Function to handle read operation for a client
void handle_read(ClientContext *client) {
    ssize_t bytes_read = read(client->fd, client->buffer + client->buffer_len, BUFFER_SIZE - client->buffer_len);
    if (bytes_read == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read");
            close(client->fd);
            free(client);
        }
    } else if (bytes_read == 0) {
        // Connection closed by client
        close(client->fd);
        free(client);
    } else {
        client->buffer_len += bytes_read;
        // Process received data (e.g., echo back to the client)
        printf("Received from client (socket descriptor %d): %s\n", client->fd, client->buffer);
        // Echo back to the client
        write(client->fd, client->buffer, client->buffer_len);
        // Reset buffer
        memset(client->buffer, 0, BUFFER_SIZE);
        client->buffer_len = 0;
    }
}

// Function to create and initialize a client context
ClientContext *create_client_context(int sockfd) {
    ClientContext *client = malloc(sizeof(ClientContext));
    if (client == NULL) {
        perror("malloc");
        return NULL;
    }
    client->fd = sockfd;
    memset(client->buffer, 0, BUFFER_SIZE);
    client->buffer_len = 0;
    return client;
}

// Main function
int main() {
    int sockfd, epollfd, nfds;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    struct epoll_event ev, events[MAX_EVENTS];

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    // Set socket to non-blocking mode
    if (set_nonblocking(sockfd) == -1) {
        close(sockfd);
        return 1;
    }

    // Initialize server address
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8080);

    // Bind socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        return 1;
    }

    // Create epoll instance
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        close(sockfd);
        return 1;
    }

    // Add sockfd to epoll
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("epoll_ctl: sockfd");
        close(sockfd);
        close(epollfd);
        return 1;
    }

    printf("Server started. Listening on port 8080...\n");

    // Event loop
    while (1) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            close(sockfd);
            close(epollfd);
            return 1;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sockfd) {
                // Accept new connection
                clilen = sizeof(cli_addr);
                int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd == -1) {
                    perror("accept");
                    continue;
                }
                // Set new socket to non-blocking mode
                if (set_nonblocking(newsockfd) == -1) {
                    close(newsockfd);
                    continue;
                }
                // Add new socket to epoll
                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = create_client_context(newsockfd);
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newsockfd, &ev) == -1) {
                    perror("epoll_ctl: newsockfd");
                    close(newsockfd);
                    continue;
                }
                printf("New connection accepted (socket descriptor %d)\n", newsockfd);
            } else {
                // Handle client I/O events
                ClientContext *client = (ClientContext *)events[i].data.ptr;
                if (events[i].events & EPOLLIN) {
                    handle_read(client);
                }
            }
        }
    }

    // Close socket and epoll
    close(sockfd);
    close(epollfd);

    return 0;
}
