#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

struct Params
{
    double *start;
    size_t len;
    int depth;
};

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void *merge_sort_thread(void *pv);

// Функция сливания массива
void merge(double *start, double *mid, double *end)
{
    double *res = new double[(end - start)*sizeof(*res)]; // выделяем новому память слитому массиву
    double *lhs = start, *rhs = mid, *dst = res; // инициализируем переменные
    while (lhs != mid && rhs != end) // До тех пор пока левая часть не равна серединке и правая не равна конечиной
        *dst++ = (*lhs < *rhs) ? *lhs++ : *rhs++; // Если элемент левой части меньше элемента правой части, то ставим левый элемент в новый массив и двигаем его на 1 вперёд. Иначе ставим правый элемент и дваигаем на 1 вперёд. Если прощё то просто находим наименьший элемент, ставим в массив, после этого передвигаем индекс наименьшего элемента на 1 вперёд, чтобы сравнивать след. элемент.
    while (lhs != mid) // Сюда код попадёт, если правая часть дошла до конца. В таком случаем заполняем результирующий массив до с левого элемента до середины без проверок. (так как они все меньше середины)
        *dst++ = *lhs++;
    
    memcpy(start, res, (rhs - start) * sizeof * res); // Копируем результирующий массив в нужную часть "главного массива"
    delete[] res;
}

// Функция для разделения массива на части
void merge_sort_mt(double *start, size_t len, int depth)
{
    if (len < 2) // Если размер части < 2 то заканчиваем разделение
        return;

    if (depth <= 0 || len < 4) // Если потоков не осталось или размер части меньше 4
    {
        merge_sort_mt(start, len/2, 0); // Разделяем часть на левую
        merge_sort_mt(start+len/2, len-len/2, 0); // И правую
    }
    else
    {
        struct Params params = { start, len/2, depth/2 }; // Собираем параметры по размеру делённому на 2 и глубине на 2 (разделяем массив на 2 части) (это левая часть)
        pthread_t thrd;
        pthread_mutex_lock(&mtx);
        printf("Starting subthread...\n"); // Выводим сообщение
        pthread_mutex_unlock(&mtx);
        pthread_create(&thrd, NULL, merge_sort_thread, &params); // Создаём поток разделения левой разделённой части массива
        merge_sort_mt(start+len/2, len-len/2, depth/2); // Разделяем правую часть дальше
        pthread_join(thrd, NULL);
        pthread_mutex_lock(&mtx);
        printf("Finished subthread.\n");
        pthread_mutex_unlock(&mtx);
    }
    
    merge(start, start+len/2, start+len); // Эта функция вызовется когда получим размер близким к 2. Сливаем части массива.
}

// Новый поток разделения части массива
void *merge_sort_thread(void *pv)
{
    Params *params = (Params*)pv;
    merge_sort_mt(params->start, params->len, params->depth);
    return pv;
}

// Запускает сортировку массива start размера len в 4 потока
void merge_sort(double *start, size_t len)
{
    merge_sort_mt(start, len, 4);
}