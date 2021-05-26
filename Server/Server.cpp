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

void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

void create_connection(in_port_t port, int sockfd)
{
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error on binding!");
}

// Цикл обработки команд сервера
void* handle_commands_thr(void* params)
{
    while (true)
    {
        char command[64];
        scanf("%s", command); // получаем команду с консоли
        if (strcmp(command, "-disconnect") == 0) // сравниваем со строками (команда отключения клиента)
        {
            char socket[2];
            scanf("%s", socket); // получаем сокет с консоли
            int sock = atoi(socket); // переводим в число
            close(sock); // закрывает соединение
            continue;
        }
        else if (strcmp(command, "-exit") == 0) // команда выхода
        {
            close(sockfd);
            exit(0);
        }
        else if (strcmp(command, "-help") == 0) // команда вывода помощи
        {
            printf("\n");
            printf("-disconnect *client fd*   Disconnects client with given fd\n");
            printf("-exit   Closes server connection and exits a program\n");
            printf("-help   Shows available commands\n");
            printf("\n");
        }
        else // если неизвестная команда
        {
            printf("Unknown command! Print -help to get help\n");
        }
    }
}

// Поток обработки подключённого клиента
void* client_handle(void* params)
{
    int client_sock = *(int*)params; // переводим сокет из типа void* в int
    
    while (true)
    {
        int fail_cnt = 0; // Переменная ошибок
        size_t array_size = 0; // Размер массива
        size_t ret = recv(client_sock, &array_size, sizeof(size_t), 0); // Получаем от клиента размер массива для сортировки
        clock_t start, stop; // Переменные времени

        if (ret > 0) // Проверка на всякий случай что переменная действительно дошла (может не дойти если клиент отключился)
        {
            double numbers[array_size]; // Массив чисел
            size_t ret = recv(client_sock, numbers, sizeof(double) * array_size, 0); // Получаем массив по размеру массива * на размер double
            printf("Got unsorted array from %i client: \n", client_sock);
            for (int i = 0; i < array_size; i++) printf("%f ", numbers[i]); // Выводим неотсортироованный массив
            printf("\n");

            start = clock(); // Узнаём время начала сортировки
            merge_sort(numbers, array_size); // Запускаемсортировку
            stop = clock(); // Узнаём время конца сортировки
            
            printf("Sorted array to %i client: \n", client_sock);
            for (int i = 0; i < array_size; i++) printf("%f ", numbers[i]); // Выводим отсортированный массив
            printf("\n");

            printf("Sorted for %f seconds\n\n", (double)(stop - start) / CLOCKS_PER_SEC); // Вывод времени сортировки

            if (send(client_sock, numbers, sizeof(double) * array_size, 0) == -1) // Отправляем отсортированный массив клиенту с проверкой на его отключение
            {
                printf("Client %i connection lost!\n", client_sock);
                close(client_sock);
                pthread_exit(params);
            }
        }
        
        fail_cnt++; // Если верхняя проверка не прошла то увеличиваем переменную
        if (fail_cnt == 10) // Если достигла 10 то отключаем клиента и выходим из потока
        {
            printf("Lost connection with %i client. Disconnecting him.", client_sock);
            close(client_sock);
            pthread_exit(params);
        }
        sleep(0.5); // Ждём 0.5 сек 
    }
}

// Поток обработки подключения клиентов
void* client_connect_handle(void*)
{
    sockaddr_in cli_addr; 
    socklen_t clilen = sizeof(cli_addr); // переменные для адреса клиента
    int client_sock; // сокет клиента
    while (true) // бекс цилк
    {
        client_sock = accept(sockfd, (sockaddr*)&cli_addr, &clilen); // принимаем клиента (эта ф-ция сама застопится пока не получит клиента)
        if (client_sock < 0) error("Error on accept\n"); // если ошибка получения сокета выводим сообщение
        printf("Client with %i sock connected\n", client_sock); // сообщеие о том что клиент подключился
        pthread_t thread;
        pthread_create(&thread, 0, client_handle, (void*)&client_sock); // создаём поток для обработки клиента с полученным сокетом в кач-ве аргумента
    }
    close(client_sock);
}

int main(int argc, char* argv[])
{
    if (argc < 2) // argc - Кол-во аргументов командной строки. Выводим сообщение, если их меньше 2 (1 всегда есть)
    {
        printf("You have to provide server port!\n");
        exit(0);
    }
    in_port_t portno = atoi(argv[1]); // переводим порт из командной строки в численную переменную

    sockfd = create_socket(); // создаём сокет
    create_connection(portno, sockfd); // создаём соединение
    if (listen(sockfd, 5) < 0) error("Error on listening start"); // начинаем принимать клиентов (5 штук маккс)

    printf("Server started on %i port\n", portno);
    pthread_t handle[2]; // переменные для потоков
    pthread_create(&handle[0], 0, client_connect_handle, 0); // запускаем поток обработки подключения клиентов
    pthread_create(&handle[1], 0, handle_commands_thr, 0); // запускаем поток обработки команд сервера
    pthread_join(handle[1], 0); // присоединяемся к потоку команд сервера (чтобы программа не завершалась)
}
