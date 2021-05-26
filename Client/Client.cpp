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
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

using namespace std;
int sockfd;

// Структура для передачи аргумента в функцию, которые требуют void*
// Функции потоков обязательно принимают void* и возвращают void*, там мы и будем их использовать
// Имеет массив и размер массива
struct ArrayParams
{
    double* array;
    size_t size = 0;
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

hostent* get_host_by_ip(string ipstr)
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
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error connecting");
    return serv_addr;
}

void delete_params(ArrayParams* params)
{
    delete[] params->array;
}

// Конвертация строки в числа
// Она же и выделит память переданному массиву numbers, заполнит его, поставит arr_size на нужный 
void string_to_numbers(const string& numbers_str, double*& numbers, size_t& arr_size, bool is_delim)
{
    double* temp = new double[10000]; // Создаём temp массив размером 10000 (нужен так как не знаем размер массива в строке)
    stringstream ss(numbers_str); // Своеобразный "парсер" который умеет переводить строку в нужный формат. Инициализируем нашей строкой.
    char delim; // разделитель
    while (!ss.eof()) // до тех пор пока не кончилась строка из парсера
    {
        ss >> temp[arr_size]; // вводим число по текущему индексу
        arr_size++; // увеличиваем индекс (так же это будет размер массива, так как число индексов равно размеру)
        if (ss.eof()) break; // ещё раз проверка
        if (is_delim) ss >> delim; // если аргумент is_delim true то вводим разделитель
    }
    numbers = new double[arr_size]; // выделяем память нашему массиву по размеру массива
    memcpy(numbers, temp, sizeof(double) * arr_size); // Копируем содержимое из temp в нужный массив
    delete[] temp; // удаляем temp
}

// Функция прочитывания чисел из файла
bool file_to_numbers(string filepath, double*& numbers, size_t& arr_size)
{
    FILE* fp;
    string nums_str;
    char filebuf[filepath.size()]; // Создаём C буфер по размеру пути к файлу
    strcpy(filebuf, filepath.c_str()); // Копируем путь к файлу в C буфер
    if ((fp = fopen(filebuf, "r")) == NULL) // Открываем файл. Если не открылся
    {
        printf("Can't open %s file!\n", filebuf); // Выводим сообщение
        return false; // Возвращаем false
    }
    else // Если открылся
    {
        fseek(fp, 0L, SEEK_END); // Переходим к концу файла
        size_t file_size = ftell(fp); // Узнаём размер файла и записываем
        fseek(fp, 0L, SEEK_SET); // Переходим в начало файла

        char buff[file_size]; // C буфер

        if (fread(buff, file_size, 1, fp) <= 0) // Читаем файл по размеру файла с проверкой
        {
            printf("Error file reading!\n");
            return false;
        } 
        
        nums_str = buff; // записываем буфер в строку
        nums_str.erase(remove(nums_str.begin(), nums_str.end(), '\n'), nums_str.end()); // удаляем из строки все '\n' (переносы строк)
    }
    
    string_to_numbers(nums_str, numbers, arr_size, false); // Вызывает функцию перевода строки чисел в массив с аргументов delim - false (так как в файле числа представлены в виде 15.0 13.0 10.0)
    return true;
}

// Функция отправки несортированного массива, получения сортированного и его вывода
void* send_and_recieve(void* params)
{
    ArrayParams* arr_params = (ArrayParams*)params; // Приводим аргумент к нужному типу
    
    printf("Unsorted numbers: \n");
    for (int i = 0; i < arr_params->size; i++) printf("%f ", arr_params->array[i]); // Выводим несортированный массив
    printf("\n");

    size_t size = arr_params->size;
    send(sockfd, &size, sizeof(size_t), 0); // Отправляем на сервер размер массива
    send(sockfd, arr_params->array, sizeof(double) * arr_params->size, 0); // Отправляем на сервер массив пакетом размером размер массива * размер double
    
    recv(sockfd, arr_params->array, sizeof(double) * arr_params->size, 0); // Получаем отсортированный массив в том же размере
    
    printf("Sorted numbers: \n");
    for (int i = 0; i < arr_params->size; i++) printf("%f ", arr_params->array[i]); // Пишем сортированный массив
    printf("\n");

    delete_params(arr_params);

    pthread_exit(nullptr); // Выходим из функции
}

// Функция обработки команды
void handle_command(const string cmd)
{
    char command[32];
    strcpy(command, cmd.c_str()); // Копируем из формата string в формат массива char

    if (strcmp(command, "-file") == 0) // Если -file
    {
        char filename[64];
        scanf("%s", filename); // получаем имя файла
        ArrayParams params; // создаём параметры
        if (file_to_numbers(filename, params.array, params.size))// если файл открылся и прочитался
        {
            pthread_t thr;
            pthread_create(&thr, 0, send_and_recieve, (void*)&params); // то отправить массив на сервер в потоке
        }
    }
    else if (strcmp(command, "-rand") == 0) // Если -rand
    {
        char cntbuf[10], minbuf[10], maxbuf[10]; // Прочитываем кол-во элементов минимальный элемент и максимальный элемент
        scanf("%s", cntbuf);
        scanf("%s", minbuf);
        scanf("%s", maxbuf);
        
        int count = atoi(cntbuf); // Переводим в численные значения
        int min = atoi(minbuf);
        int max = atoi(maxbuf);

        ArrayParams params; // Создаём параметры
        params.array = new double[count]; // Выделяем память для массива по размеру count 
        params.size = count; // Ставим size в параметры
        for (int i = 0; i < count; i++)
        {
            params.array[i] = min + rand() % max; // Рандомим числа массива в пределах min&max
        }
        pthread_t thr;
        pthread_create(&thr, 0, send_and_recieve, (void*)&params); // то отправить массив на сервер в потоке
    }
    else if (strcmp(command, "-nums") == 0) // Если -nums
    {
        char numbers[10000];
        scanf("%s", numbers); // Сканируем строку чисел
        ArrayParams params; // Параметры
        string_to_numbers(numbers, params.array, params.size, true); // Вызываем функцию перевода строки чисел в числа с аргументом delim - true (числа записаны в формате 15.0|13.0|10.0)
        pthread_t thr;
        pthread_create(&thr, 0, send_and_recieve, (void*)&params); // то отправить массив на сервер в потоке
    }
    else if (strcmp(command, "-exit") == 0) // Выход
    {
        close(sockfd);
        exit(1);
    }
    else if (strcmp(command, "-help") == 0) // Помощь
    {
        printf("\n");
        printf("Command list: \n");
        printf("-file *filename*   Sorts array numbers from given file.\n");
        printf("-rand *count* *min* *max*   Sorts array of *count* elements randomized between *min* and *max*.\n");
        printf("-nums *num1|nums2|num3...* Sorts array of given numbers\n");
        printf("-exit   Closes the connection and exits from program\n");
        printf("-help   Shows available commands\n");
        printf("\n");
    }
    else // Неизвестная команда
    {
        printf("Unknown command! Print -help to get help\n");
    }
}

// Поток обработки команд
void* handle_commands_thr(void*)
{
    char command[32];
    while (true)
    {
        scanf("%s", command); // получаем команду с терминала
        handle_command(command); // вызываем ф-цию обработки команды
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) error("You have to provide ip:port!\n");
    string addr = argv[1];
    string ip = addr.substr(0, addr.find(':'));
    if (ip.size() == addr.size())
    {
        printf("Bad argument! Please provide adress as ip:port!\n");
        exit(0);
    }
    string port_str = addr.substr(ip.size() + 1, addr.size() - 1);
    in_port_t portno = atoi(port_str.c_str());

    hostent *server;
    sockfd = create_socket();
    server = get_host_by_ip(ip);
    sockaddr_in serv_addr = server_connect(portno, sockfd, server);

    printf("Connected! \n");
    // if (argc >= 3)
    // {
    //     string arg = argv[3];
    //     handle_command(argv[3]);
    // }
    // else
    // {
        printf("Start typing commands to use this program functionality.\n Type -help to get commands list\n");
    //}

    pthread_t thr;
    pthread_create(&thr, 0, handle_commands_thr, 0);
    pthread_join(thr, 0);
}