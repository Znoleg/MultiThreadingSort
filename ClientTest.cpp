#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>
#include <array>

using namespace std;
#define STD_PORT 2048
int sockfd;

struct ArrayParams
{
    double* array;
    size_t size;
};

void error(string msg)
{
    error(msg.c_str());
}

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

hostent* get_host_by_hostname(string name = "")
{
    if (name == "")
    {
        char buff[64];
        gethostname(buff, 63);
        return gethostbyname(buff);
    }
    else return gethostbyname(name.c_str());
}

hostent* get_host_by_ip(string ipstr = "127.0.0.1")
{
    in_addr ip;
    hostent *hp;
    if (!inet_aton(ipstr.c_str(), &ip)) error("Can't parse IP address!");
    if ((hp = gethostbyaddr((const void*)&ip, sizeof(ip), AF_INET)) == NULL) error("No server associated with " + ipstr);
    return hp;
}

sockaddr_in server_connect(in_port_t port, int sockfd, hostent* server)
{
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    bcopy((char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);
    //if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error ("Error on binding!");
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error connecting");
    return serv_addr;
}

void string_to_numbers(const string& numbers_str, double* numbers, size_t& arr_size)
{
    double* temp = new double[10000];
    size_t arr_size;
    stringstream ss(numbers_str);
    while (true)
    {
        ss >> temp[arr_size];
        if (ss.eof()) break;
        arr_size++;
    }
    numbers = new double[arr_size];
    memcpy(numbers, temp, sizeof(double) * arr_size);
    delete[] temp;
}

void file_to_numbers(const char* filepath, double* numbers, size_t& arr_size)
{
    FILE* fp;
    string numbers_str;
    if ((fp = fopen(filepath, "r")) == NULL)
    {
        printf("Can't open %s file!", filepath);
        return;
    }
    else
    {
        while (fread(&numbers_str, sizeof(double), sizeof(double), fp) == sizeof(double));
    }
    
    string_to_numbers(numbers_str, numbers, arr_size);
}

string file_to_numbers_str(const char* filepath)
{
    FILE* fp;
    string numbers;
    double number;
    if ((fp = fopen(filepath, "r")) == NULL)
    {
        printf("Can't open %s file!", filepath);
        return;
    }
    else
    {
        while (fread(&number, sizeof(double), sizeof(double), fp) == sizeof(double))
        {
            numbers += to_string(number) + ' ';
        }
    }
    return numbers;
}

void* send_and_recieve(void* params)
{
    ArrayParams* arr_params = (ArrayParams*)params;
    printf("%Unsorted numbers: %s\n");
    for (int i = 0; i < arr_params->size; i++) printf("%f ", arr_params[i]);

    send(sockfd, &arr_params->size, sizeof(size_t), 0);
    recv(sockfd, arr_params->array, sizeof(double) * arr_params->size, 0);
    
    printf("Sorted numbers: %s\n");
    for (int i = 0; i < arr_params->size; i++) printf("%f ", arr_params[i]);
}

int main(int argc, char *argv[])
{
    array<int, 10> arr;

    int portno = 80, n;
    if (argc == 4 && argv[2] == "file") // если числа из файла с командной строки
    {
        ArrayParams params;
        file_to_numbers(argv[3], params.array, params.size);
        send_and_recieve((void*)&params);
    }
    else if (argc == 3)
    {
        send_and_recieve((void*)&argv[2]);
    }
    hostent *server;
    sockfd = create_socket();
    server = get_host_by_hostname();
    sockaddr_in serv_addr = server_connect(portno, sockfd, server);

    char buf[256] = "Simple message";
}