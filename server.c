/* A simple server in the internet domain using TCP - The port number is passed
 * as an argument */
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define MAXCONNECTION 100 // maximum number of simultaneously connection allowed
const char CONTENTDIR[] = "./contentdir"; // this is the directory where keep
                                          // all the files for requests
void error(const char *msg) {
  perror(msg);
  exit(1);
}

void httpWorker(int *); // This function will handle request
char *fType(char *);
char *responseHeader(int, char *); // function that builds response header

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");
  listen(sockfd, MAXCONNECTION);
  while (1) {
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");
    httpWorker(&newsockfd); // worker to fulfillthe request
  }

  close(sockfd);
  return 0;
}

void httpWorker(int *sockfd) { // sockfd contains all the information
  int newsockfd = *sockfd;     // create a local variable for sockfd
  char buffer[256];            // we will read the data in this buffer
  char *token;        // local variable to split the request to get the filename
  bzero(buffer, 256); // intialize the buffer data to zero
  char fileName[50];
  char homedir[50];
  char *type;
  strcpy(homedir, CONTENTDIR); // directory where files are stored.
  char *respHeader;            // response header
  // start reading the message from incoming conenction
  if (read(newsockfd, buffer, 255) < 0)
    error("ERROR reading from socket");
  // get the requested file part of the request
  token = strtok(buffer, " "); // split string into token seperated by " "
  token = strtok(NULL,
                 " "); // in this go we read the file name that needs to be sent
  strcpy(fileName, token);

  // get the complete filename
  if (strcmp(fileName, "/") ==
      0) // if filename is not provided then we will send index.html
    strcpy(fileName, strcat(homedir, "/index.html"));
  else
    strcpy(fileName, strcat(homedir, fileName));
  type = fType(fileName); // get file type
  // open file and ready to send
  FILE *fp;
  int file_exist = 1;
  fp = fopen(fileName, "r");
  if (fp == NULL)
    file_exist = 0;
  respHeader = responseHeader(file_exist, type);
  if ((send(newsockfd, respHeader, strlen(respHeader), 0) == -1) ||
      (send(newsockfd, "\r\n", strlen("\r\n"), 0) == -1))
    perror("Failed to send bytes to client");

  free(respHeader); // free the allocated memory (note: the memory is allocated
                    // in responseheader function)

  if (file_exist) {
    char filechar[1];
    while ((filechar[0] = fgetc(fp)) != EOF) {
      if (send(newsockfd, filechar, sizeof(char), 0) == -1)
        perror("Failed to send bytes to client");
    }
  } else {
    if (send(newsockfd,
             "<html> <HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY>Not "
             "Found</BODY></html> \r\n",
             100, 0) == -1)
      perror("Failed to send bytes to client");
  }

  close(newsockfd);
}

// function below find the file type of the file requested
char *fType(char *fileName) {
  char *type;
  char *filetype =
      strrchr(fileName, '.'); // This returns a pointer to the first occurrence
                              // of some character in the string
  if ((strcmp(filetype, ".htm")) == 0 || (strcmp(filetype, ".html")) == 0)
    type = "text/html";
  else if ((strcmp(filetype, ".jpg")) == 0)
    type = "image/jpeg";
  else if (strcmp(filetype, ".gif") == 0)
    type = "image/gif";
  else if (strcmp(filetype, ".txt") == 0)
    type = "text/plain";
  else
    type = "application/octet-stream";

  return type;
}

// buildresponseheader
char *responseHeader(int filestatus, char *type) {
  char statuscontent[256] = "HTTP/1.0";
  if (filestatus == 1) {
    strcat(statuscontent, " 200 OK\r\n");
    strcat(statuscontent, "Content-Type: ");
    strcat(statuscontent, type);
    strcat(statuscontent, "\r\n");
  } else {
    strcat(statuscontent, "404 Not Found\r\n");
    // send a blank line to indicate the end of the header lines
    strcat(statuscontent, "Content-Type: ");
    strcat(statuscontent, "NONE\r\n");
  }
  char *returnheader = malloc(strlen(statuscontent) + 1);
  strcpy(returnheader, statuscontent);
  return returnheader;
}
