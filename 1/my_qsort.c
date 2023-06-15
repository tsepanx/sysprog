//
// Created by void on 6/12/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_SIZE 10000

struct int_arr {
    int* start;
    int length;
};

void print_arr(struct int_arr arr) {
    int* ptr = arr.start;
    printf("Arr: ");
    for (int i = 0; i < arr.length; ++i) {
        printf("%d ", *ptr);
        ptr++;
    }
    printf(" |\n");
}

void my_qsort(int* arr, int i_start, int i_end) {
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
//            printf("%d %d ", i, j);

            i++;
            j--;

//            print_arr(start, 9);
        }
    } while (i <= j);

    if (j > i_start) {
        my_qsort(arr, i_start, j);
    }
    if (i < i_end) {
        my_qsort(arr, i, i_end);
    }
}

void sort_int_arr(struct int_arr ia) {
    my_qsort(ia.start, 0, ia.length - 1);
}

struct int_arr arr_from_file(char* fname) {
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

    return (struct int_arr) {arr, arr_size};
}

void arr_to_file(struct int_arr arr, char* fname) {
    FILE* fout = fopen(fname, "w");

    for (int j = 0; j < arr.length; ++j) {
        fprintf(fout, "%d ", arr.start[j]);
    }

    fclose(fout);
}

struct int_arr single_file_sort(char* fname) {
    struct int_arr ia = arr_from_file(fname);

    sort_int_arr(ia);

    return ia;
}

struct int_arr create_int_arr(int of_size) {
    return (struct int_arr) { .start = malloc(of_size * sizeof(int)), .length = of_size };
}

struct int_arr merge_two_arrays(struct int_arr* ia1, struct int_arr* ia2) {
    int* p1 = ia1->start;
    int* p2 = ia2->start;

    int* end1 = ia1->start + ia1->length;
    int* end2 = ia2->start + ia2->length;

    int res_length = ia1->length + ia2->length;
    struct int_arr result_arr = create_int_arr(res_length);

    int i = 0;
    while (p1 < end1 || p2 < end2) {
        if ((*p1 < *p2 || p2 >= end2) && p1 < end1) {
            result_arr.start[i] = *p1;
            p1++;
        } else if ((*p1 >= *p2 || p1 >= end1) && p2 < end2){
            result_arr.start[i] = *p2;
            p2++;
        } else {
            fprintf(stderr, "Err :(");
            exit(-1);
        }
        i++;
    }

    return result_arr;
}

struct int_arr multiple_files_sort(char** fnames, int fnames_cnt) {
    struct int_arr result_arr;

    for (int i = 0; i < fnames_cnt; ++i) {
        char* fname_i = fnames[i];
        struct int_arr ia_i = arr_from_file(fname_i);

        sort_int_arr(ia_i);

        if (i == 0) {
            result_arr = ia_i;
        } else {
            // TODO: Optimize to tree merging
            result_arr = merge_two_arrays(&result_arr, &ia_i);
        }
    }

    return result_arr;
}

int main(int argc, char **argv) {

    struct int_arr res;
    if (argc == 2) {
        char* fname;
        fname = argv[1];

        res = single_file_sort(fname);
    } else if (argc > 2) {
        res = multiple_files_sort(++argv, argc - 1);
    } else {
        fprintf(stderr, "Wrong arguments\n");
        return -1;
    }

    print_arr(res);
    arr_to_file(res, "out1.txt");

    return 0;
}