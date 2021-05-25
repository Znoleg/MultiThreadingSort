#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <algorithm>
#include <vector>

using namespace std;
bool thread;

template <typename T>
struct IteratorPair
{
    typename vector<T>::iterator begin;
    typename vector<T>::iterator end;
};

template <typename T>
int native_quick_sort(IteratorPair<T>& pair);

// template <typename T>
// void swap(T* arg1, T* arg2)
// {
//     T* temp = arg2;
//     arg1 = arg2;
//     arg2 = temp;
// }

template <typename T>
void* new_sort_thread(void* arg)
{
    thread = true;
    IteratorPair<T>& pair = *(IteratorPair<T>*)arg;
    native_quick_sort(pair);
    thread = false;
    pthread_exit(0);
}

template <typename T>
int native_quick_sort(IteratorPair<T>& pair)
{
    auto begin = pair.begin;
    auto end = pair.end;
    auto const sz = pair.end - pair.begin;
    if (sz <= 1) return 0;

    auto pivot = pair.begin + sz/2;
    auto const pivot_v = *pivot;

    auto right = end - 1;
    std::swap(*pivot, *(end - 1));
    auto p = partition(begin, end, [&](const T& a) {return a < pivot_v;});
    std::swap(*p, *(end - 1));

    if (sz > 8)
    {
        pthread_t thr;
        pthread_create(&thr, 0, new_sort_thread<T>, (void*)&pair);
        printf("%s\n", "New sorting thread");
        IteratorPair<T> pair{p + 1, end};
        native_quick_sort(pair);
    }
    else
    {
        IteratorPair<T> pair1{begin, p};
        IteratorPair<T> pair2{p + 1, end};
        native_quick_sort(pair1);
        native_quick_sort(pair2);
    }

    return 0;
}

int main()
{
    vector<double> unsorted{20, 60, 34, 12, 34, 856, 124, 215, 643, 745 ,856,43,325, 3,432 ,2342343};
    IteratorPair<double> to_sort = {unsorted.begin(), unsorted.end()};
    native_quick_sort<double>(to_sort);
    while (thread);
    for (double value : unsorted)
    {
        printf("%f ", value);
    }
}