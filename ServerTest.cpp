#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <threads.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sstream>
#include "Sorter.cpp"

using namespace std;
int sockfd;

void error(char* msg)
{
    perror(msg);
    exit(1);
}

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

sockaddr_in create_connection(in_port_t port, int sockfd)
{
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error ("Error on binding!");
}

void start_listening(int sockfd)
{
    if (listen(sockfd, 5) < 0) error("Error on listening start");
}

void* handle_commands(void* params)
{
    while (true)
    {
        char command[64];
        scanf("%s", command);
        char* found;
        found = strstr(command, "disconnect");
        if (found != NULL)
        {
            char socket[2];
            strcpy(socket, found + 1);
            int sock = atoi(socket);
            close(sock);
            continue;
        }
        found = strstr(command, "exit");
        if (found != NULL)
        {
            close(sockfd);
            exit(0);
        }
        //if (char p = (strstr(command, "disconnect")) != 0)
    }
}

void* client_handle(void* params)
{
    int client_sock = *(int*)params;
    
    size_t array_size = 0;
    recv(client_sock, &array_size, sizeof(size_t), 0);
    double numbers[array_size];

    recv(client_sock, numbers, sizeof(double) * array_size, 0);

    printf("Got unsorted numbers from %i client: \n", client_sock);
    for (int i = 0; i < array_size; i++) printf("%f ", numbers[i]);

}

int main(int argc, char* argv[])
{
    in_port_t portno = 2080;
    if (argc == 2)
    {
        portno = atoi(argv[1]);
    }

    int client_sock;
    socklen_t clilen;
    sockaddr_in serv_addr, cli_addr;
    
    sockfd = create_socket();
    create_connection(portno, sockfd);
    start_listening(sockfd);

    clilen = sizeof(cli_addr);

    printf("Server started\n");
    while (true)
    {
        client_sock = accept(sockfd, (sockaddr*)&cli_addr, &clilen);
        if (client_sock < 0) error("Error on accept\n");
        printf("Client with %i sock connected\n", client_sock);
        pthread_t thread;
        pthread_create(&thread, 0, client_handle, (void*)&client_sock);
    }

    close(client_sock);
}
