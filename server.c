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

void httpWorker(int *sockfd) {
  int newsockfd = *sockfd;
  char buffer[8192]; // buffer lớn để chứa header + body (nếu nhỏ)
  char fileName[256];
  char homedir[256];
  char method[8];
  char *type;
  char *respHeader;

  strcpy(homedir, CONTENTDIR);
  int total_read = 0, n;

  // Đọc đến khi thấy \r\n\r\n (kết thúc header)
  while ((n = read(newsockfd, buffer + total_read, sizeof(buffer) - total_read - 1)) > 0) {
    total_read += n;
    buffer[total_read] = '\0';
    if (strstr(buffer, "\r\n\r\n")) break;
  }

  if (total_read <= 0) {
    close(newsockfd);
    return;
  }

  // In ra header
  printf("Buffer (header + có thể 1 phần body):\n%.*s\n", total_read, buffer);

  // Giữ lại buffer ban đầu để sử dụng sau này
  char headerCopy[8192];
  strcpy(headerCopy, buffer);

  // Parse method và file name
  char *token = strtok(buffer, " ");
  if (!token) return;
  strcpy(method, token);

  token = strtok(NULL, " ");
  if (!token) return;
  strcpy(fileName, token);

  printf("Method: %s\n", method);
  printf("File name: %s\n", fileName);

  if (strcmp(method, "GET") == 0) {
    if (strcmp(fileName, "/") == 0)
      strcpy(fileName, strcat(homedir, "/index.html"));
    else
      strcpy(fileName, strcat(homedir, fileName));

    type = fType(fileName);
    FILE *fp = fopen(fileName, "r");
    int file_exist = fp != NULL;

    respHeader = responseHeader(file_exist, type);
    send(newsockfd, respHeader, strlen(respHeader), 0);
    send(newsockfd, "\r\n", 2, 0);
    free(respHeader);

    if (file_exist) {
      char filechar[1];
      while ((filechar[0] = fgetc(fp)) != EOF)
        send(newsockfd, filechar, 1, 0);
      fclose(fp);
    } else {
      send(newsockfd, "<html><head><title>404</title></head><body>Not Found</body></html>", 71, 0);
    }
  }

  else if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
    strcpy(fileName, strcat(homedir, fileName));
    FILE *fp;

    if (strcmp(method, "POST") == 0)
      fp = fopen(fileName, "a");
    else
      fp = fopen(fileName, "w");

    if (fp == NULL) {
      respHeader = responseHeader(0, "text/plain");
      send(newsockfd, respHeader, strlen(respHeader), 0);
      send(newsockfd, "\r\n", 2, 0);
      free(respHeader);
    } else {
      // Tìm Content-Length
      int content_length = 0;
      char *cl_ptr = strcasestr(headerCopy, "Content-Length:");
      if (cl_ptr) {
        sscanf(cl_ptr, "Content-Length: %d", &content_length);
        printf("Content-Length: %d\n", content_length);
      } else {
        printf("Không tìm thấy Content-Length\n");
      }

      // Tìm phần body bắt đầu sau \r\n\r\n
      char *body = strstr(headerCopy, "\r\n\r\n");
      if (body != NULL) {
        // Bỏ qua "\r\n\r\n" để trỏ vào phần body thực sự
        body += 4;
        printf("Body: %s\n", body);
        int body_len = strlen(body);

        // Ghi phần body vào file
        fwrite("\n", 1, 1, fp);
        fwrite(body, 1, body_len, fp);
      } else {
        printf("Không tìm thấy phần body sau header.\n");
      }

      fclose(fp);
      respHeader = responseHeader(1, "text/plain");
      send(newsockfd, respHeader, strlen(respHeader), 0);
      send(newsockfd, "\r\n", 2, 0);
      send(newsockfd, "Success\n", 8, 0);
      free(respHeader);
    }
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
