#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h> 
#include <arpa/inet.h>

#define BUF_SIZE 256

int command(int sock, char* buf, char* command, char* userSeeStr, int userInput) {
    int goodAnswer = 0;
    int commandLen = strlen(command);
    memset(buf, 0, BUF_SIZE);
    strcpy(buf, command);
    printf("%s", userSeeStr);
    if (userInput) {
        fgets(buf + commandLen, BUF_SIZE-1-commandLen, stdin);
    }
    write(sock, buf, strlen(buf));
    memset(buf, 0, BUF_SIZE);
    read(sock, buf, BUF_SIZE-1);
    goodAnswer = sscanf(buf, "+OK %s", buf);
    if (goodAnswer < 1) {
        int badAnswer = 0;
        badAnswer = sscanf(buf, "-ERR %s", buf);
        if (badAnswer < 1)
            printf("Bad formatted server reply:\n");
        printf("%s\n", buf);
        return -1;
    }
    printf("%s\n", buf);
    return 0;
}

int login(int sock, char* buf)
{
    static char* userCommand = "USER ";
    static char* passwordCommand = "PASS ";
    if (command(sock, buf, userCommand, "Enter the username: ", 1) < 0) {
        return -1;
    }
    if (command(sock, buf, passwordCommand, "Enter the password: ", 1) < 0) {
        return -1;
    }
    return 0;
}

int quit(int sock, char* buf)
{
    int goodAnswer = 0;
    static char* userCommand = "QUIT";
    if (command(sock, buf, userCommand, "Bye!\n", 0) < 0) {
        exit(-1);
    }
    exit(0);
}

int stat(int sock, char* buf)
{
    static char* userCommand = "STAT";
    if (command(sock, buf, userCommand, "All your mail contains: ", 0) < 0) {
        return -1;
    }
    return 0;
}

int list(int sock, char* buf)
{
    static char* userCommand = "LIST ";
    if (command(sock, buf, userCommand, "Enter the message id (if empty get all): ", 1) < 0) {
        return -1;
    }
    return 0;
}

int retr(int sock, char* buf)
{
    static char* userCommand = "RETR ";
    if (command(sock, buf, userCommand, "Enter the reading message id: ", 1) < 0) {
        return -1;
    }
    return 0;
}

int main(int argc, char ** argv)
{
    int sock, port;
    struct sockaddr_in serv_addr;
    //struct hostent *server;
    char buf[BUF_SIZE];

    if (argc < 3) 
    {
       fprintf(stderr,"usage: %s <hostname> <port_number>\n", argv[0]);
       return EXIT_FAILURE;
    }
    port = atoi(argv[2]);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
      printf("socket() failed: %d", errno);
      return EXIT_FAILURE;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0){
        printf("setsockopt(SO_REUSEADDR) failed\n");
        return EXIT_FAILURE;
    }
    //server = gethostbyname(argv[1]);
    /*if (server == NULL) 
    {
      printf("Host not found\n");
      return EXIT_FAILURE;
    }*/
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    //strncpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
      printf("connect() failed: %d", errno);
      return EXIT_FAILURE;
    }

    char c;
    while(1) {
        printf("Enter the command ('?' for help): ");
        fscanf(stdin, "%c", &c);
        getchar();
        switch(c) {
        case 'a':
            login(sock, buf);
            break;
        case 'l':
            list(sock, buf);
            break;
        case 's':
            stat(sock, buf);
            break;
        case 'r':
            retr(sock, buf);
            break;
        case 'q':
            quit(sock, buf);
            return 0;
        case '?':
            printf("a - authorization\n");
            printf("l - list of messages in mail\n");
            printf("s - statistic of the message\n");
            printf("r - retrieve the message\n");
            printf("q - quit\n");
            break;
        default:
            printf("Unknown command, for help type '?'\n");
        }
    }
    
    close(sock);
    return 0;
}
