#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CONNECTIONS 100
#define QUEUE_SIZE 10

const char CONTENTDIR[] = "./contentdir"; // Content directory for static files
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct {
    int sockfd;
} request_t;

request_t queue[QUEUE_SIZE];
int queue_front = 0;
int queue_rear = 0;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void httpWorker(int sockfd) {
    char buffer[8192];
    int n;
    char fileName[256];
    char method[8];

    // Read HTTP request
    n = read(sockfd, buffer, sizeof(buffer)-1);
    if (n <= 0) {
        close(sockfd);
        return;
    }
    buffer[n] = '\0';

    printf("Received request: %s\n", buffer);

    // Parse method and file name
    sscanf(buffer, "%s %s", method, fileName);
    if (strcmp(method, "GET") == 0) {
        // Process GET request here (similar to Phase 1)
        // Respond with file content or 404 if not found
        send(sockfd, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, world!", 60, 0);
    }

    close(sockfd);
}

void* workerThread(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        
        while (queue_front == queue_rear) {
            // Queue is empty, wait for request
            pthread_cond_wait(&cond, &mutex);
        }
        
        // Get request from queue
        int sockfd = queue[queue_front].sockfd;
        queue_front = (queue_front + 1) % QUEUE_SIZE;

        pthread_mutex_unlock(&mutex);

        printf("Worker thread %ld processing request...\n", pthread_self());

        // Process the HTTP request
        httpWorker(sockfd);
    }

    return NULL;
}

void* listenerThread(void* arg) {
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = *((int*)arg);  // Port number passed as argument
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, MAX_CONNECTIONS);

    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        // Add the new socket connection to the queue
        pthread_mutex_lock(&mutex);
        queue[queue_rear].sockfd = newsockfd;
        queue_rear = (queue_rear + 1) % QUEUE_SIZE;
        pthread_cond_signal(&cond);  // Signal worker thread
        pthread_mutex_unlock(&mutex);
    }

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "ERROR, no port or thread count provided\n");
        exit(1);
    }

    int portno = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    pthread_t listener, scheduler;
    pthread_t workers[num_threads];

    // Start listener thread
    pthread_create(&listener, NULL, listenerThread, &portno);

    // Start worker threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&workers[i], NULL, workerThread, NULL);
    }

    // Wait for threads to finish (this won't happen as threads are infinite loops)
    pthread_join(listener, NULL);
    for (int i = 0; i < num_threads; i++) {
        pthread_join(workers[i], NULL);
    }

    return 0;
}
