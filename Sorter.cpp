#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

struct Params
{
    int *start;
    size_t len;
    int depth;
};

// merge numa la syncronous
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void *merge_sort_thread(void *pv);

void merge(int *start, int *mid, int *end)
{
    int *res = (int*)malloc((end - start)*sizeof(*res));
    int *lhs = start, *rhs = mid, *dst = res;
    while (lhs != mid && rhs != end)
        *dst++ = (*lhs < *rhs) ? *lhs++ : *rhs++;
    while (lhs != mid)
        *dst++ = *lhs++;
    // copie rezultatele
    memcpy(start, res, (rhs - start) * sizeof *res);
    free(res);
}
// punctul de intrare
void merge_sort_mt(int *start, size_t len, int depth)
{
    if (len < 2)
        return;

    if (depth <= 0 || len < 4)
    {
        merge_sort_mt(start, len/2, 0);
        merge_sort_mt(start+len/2, len-len/2, 0);
    }
    else
    {
        struct Params params = { start, len/2, depth/2 };
        pthread_t thrd;
        pthread_mutex_lock(&mtx);
        printf("Starting subthread...\n");
        pthread_mutex_unlock(&mtx);
        pthread_create(&thrd, NULL, merge_sort_thread, &params);
        merge_sort_mt(start+len/2, len-len/2, depth/2);
        pthread_join(thrd, NULL);
        pthread_mutex_lock(&mtx);
        printf("Finished subthread.\n");
        pthread_mutex_unlock(&mtx);
    }
    // merge
    merge(start, start+len/2, start+len);
}
// algoritmu care baga parametrii in algoritmu de sort
void *merge_sort_thread(void *pv)
{
    Params *params = (Params*)pv;
    merge_sort_mt(params->start, params->len, params->depth);
    return pv;
}

// unirea
void merge_sort(int *start, size_t len)
{
    merge_sort_mt(start, len, 4);
}

int main()
{
    clock_t start, stop;
    static const unsigned int N = 10;
    int *data = new int[N];
    unsigned int i;
    srand(time(NULL));
    for (i=0; i<N; ++i)
    {
        data[i] = rand() % 500;
    }
    for (i=0; i<N; ++i)
    {
        printf("%i ", data[i]);
    }
    start = clock();
    merge_sort(data, N);
    for (i=0; i<N; ++i)
    {
        printf("%i ", data[i]);
    }
    //sf executiei f principale
   stop = clock();
       printf("Elapsed: %f seconds\n", (double)(stop - start) / CLOCKS_PER_SEC);
   // printf("\n");
    free(data);
    getchar();
    getchar();
    getchar();
    getchar();
    getchar();
    return 0;
}