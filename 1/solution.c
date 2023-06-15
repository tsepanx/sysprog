#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libcoro.h"

#define MAX_SIZE 10000

struct int_arr {
    int* start;
    int length;
};

struct coro_args {
    char* name;
    struct int_arr* ia;
//    unsigned long* run_time;
};

unsigned long nsec(struct timespec ts) {
    return ts.tv_sec * (unsigned) 1e9 + ts.tv_nsec;
}

unsigned long get_cur_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return nsec(ts);
}

void my_qsort(int* arr, int i_start, int i_end, unsigned long* t_hold) {
    int m = arr[(i_end + i_start) / 2];

    int i = i_start;
    int j = i_end;

    do {
        while (arr[i] < m) {
            i++;
        }

        while (arr[j] > m) {
            j--;
        }

        if (i <= j) {
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;

            i++;
            j--;

            size_t t1 = get_cur_time();

            coro_yield(); // Yield here

            size_t t2 = get_cur_time();
            size_t diff = t2 - t1;

            *t_hold += diff;
        }
    } while (i <= j);

    if (j > i_start) {
        my_qsort(arr, i_start, j, t_hold);
    }
    if (i < i_end) {
        my_qsort(arr, i, i_end, t_hold);
    }
}

void sort_int_arr(struct int_arr* ia, unsigned long* t_hold) {
    my_qsort(ia->start, 0, ia->length - 1, t_hold);
}

struct int_arr* arr_from_file(char* fname) {
    FILE* fin = fopen(fname, "r");
    if (fin == NULL) {
        fprintf(stderr, "File not found\n");
        exit(-1);
    }

    int n, i = 0, arr_size = 0;
    int* arr = malloc(MAX_SIZE * sizeof(int));

    while (fscanf(fin, "%d", &n) == 1) {
        arr[i] = n;
        i++;
        arr_size++;
    }
    fclose(fin);

//    start = realloc(start, length);

    struct int_arr* ia = malloc(sizeof(struct int_arr));
    *ia = (struct int_arr) { arr, arr_size };
    return ia;
}

void arr_to_file(struct int_arr* arr, char* fname) {
    FILE* fout = fopen(fname, "w");

    for (int j = 0; j < arr->length; ++j) {
        fprintf(fout, "%d ", arr->start[j]);
    }
    fprintf(fout, "\n");

    fclose(fout);
}

struct int_arr create_int_arr(int of_size) {
    return (struct int_arr) { .start = malloc(of_size * sizeof(int)), .length = of_size };
}

struct int_arr* merge_two_arrays(struct int_arr* ia1, struct int_arr* ia2) {
    int* p1 = ia1->start;
    int* p2 = ia2->start;

    int* end1 = ia1->start + ia1->length;
    int* end2 = ia2->start + ia2->length;

    int res_length = ia1->length + ia2->length;
    struct int_arr* result_arr = malloc(sizeof(struct int_arr));
    *result_arr = create_int_arr(res_length);

    int i = 0;
    while (p1 < end1 || p2 < end2) {
        if ((*p1 < *p2 || p2 >= end2) && p1 < end1) {
            result_arr->start[i] = *p1;
            p1++;
        } else if ((*p1 >= *p2 || p1 >= end1) && p2 < end2){
            result_arr->start[i] = *p2;
            p2++;
        } else {
            fprintf(stderr, "Err :(\n");
            exit(-1);
        }
        i++;
    }

    return result_arr;
}

struct int_arr* multiple_ia_sort(struct int_arr** ia_arr, int size) {
    struct int_arr* result_arr;

    // TODO: Optimize to bin-tree merging
    for (int i = 0; i < size; ++i) {
        if (i == 0) {
            result_arr = ia_arr[i];
        } else {
            result_arr = merge_two_arrays(result_arr, ia_arr[i]);
        }
    }

    return result_arr;
}

static int coroutine_func_f(void *arg1) {
    struct coro_args* ca = arg1;
    struct int_arr* ia = ca->ia;
    char* coro_name = ca->name;

    free(ca);

    unsigned long t_start = get_cur_time();
    unsigned long* t_hold = malloc(sizeof(unsigned long));
    *t_hold = 0;
//    unsigned long t_hold = get_cur_time();

    /* IMPLEMENT SORTING OF INDIVIDUAL FILES HERE. */
    sort_int_arr(ia, t_hold);

    size_t exec_time = get_cur_time() - t_start - *t_hold;

    struct coro* this = coro_this();
    printf("Coro %s t_hold: %f sec\n", coro_name, (double) *t_hold / 1e9);
    printf("Coro %s exec_time: %f sec\n", coro_name, (double) exec_time / 1e9);
    printf("Coro %s switch count: %lld\n", coro_name, coro_switch_count(this));

	return 0;
}

void free_int_arr(struct int_arr* ia) {
    free(ia->start);
    free(ia);
}

void print_arr(struct int_arr* arr) {
    printf("%p %p  || ", arr, arr->start);
    int* ptr = arr->start;
    printf("Arr: ");
    for (int i = 0; i < arr->length; ++i) {
        printf("%d ", *ptr);
        ptr++;
    }
    printf(" |\n");
}

int main(int argc, char **argv) {
    unsigned long start_time;

    struct timespec exec_time;
    clock_gettime(CLOCK_MONOTONIC, &exec_time);

    start_time = get_cur_time();

    int fnames_cnt = argc - 1;
    char** fnames = ++argv;

    struct int_arr** ia_arr = malloc(fnames_cnt * sizeof(struct int_arr));

	/* Initialize our coroutine global cooperative scheduler. */
	coro_sched_init();
	/* Start several coroutines. */
	for (int i = 0; i < fnames_cnt; ++i) {
		/*
		 * The coroutines can take any 'void *' interpretation of which
		 * depends on what you want. Here as an example I give them
		 * some names.
		 */

        struct int_arr* ia_i = arr_from_file(fnames[i]);
        ia_arr[i] = ia_i;

//        print_arr(ia_i);

        struct coro_args* ca_i = malloc(sizeof(struct coro_args));
        ca_i->ia = ia_i;
        ca_i->name = fnames[i];

        coro_new(coroutine_func_f, ca_i);
    }
	/* Wait for all the coroutines to end. */
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL) {
		/*
		 * Each 'wait' returns a finished coroutine with which you can
		 * do anything you want. Like check its exit status, for
		 * example. Don't forget to free the coroutine afterwards.
		 */
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}

    /* All coroutines have finished. */

    /* IMPLEMENT MERGING OF THE SORTED ARRAYS HERE. */

    struct int_arr* res_ia = multiple_ia_sort(ia_arr, fnames_cnt);

    arr_to_file(res_ia, "out1.txt");

    free_int_arr(res_ia);
    for (int i = 0; i < fnames_cnt; ++i) { free_int_arr(ia_arr[i]); }

    unsigned long finish_time;

    clock_gettime(CLOCK_MONOTONIC, &exec_time);
    finish_time = get_cur_time();

    double seconds = (double) (finish_time - start_time) / 1e9;
    printf("Overall execution time: %f sec\n", seconds);

	return 0;
}
