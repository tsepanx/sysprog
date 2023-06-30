
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define SPACE ' '
#define BAR '|'
#define LINE_BREAK '\n'
#define QUOTE_1 '\''
#define QUOTE_2 '\"'
#define BACKSLASH '\\'

//struct cmd {
//    char *name;
//    const char **argv;
//    int argc;
//};

struct string_ends {
    char* l;
    char* r; // Right after last element
};

struct cmd {
    struct string_ends name;
    struct string_ends* argv;
    int argc;
};

int se_size(struct string_ends* se) {
    return (int) (se->r - se->l);
}

void se_cpy(struct string_ends* se, char* ptr) {
    // MALLOC
    int size = se_size(se);

    ptr = malloc(size * sizeof(char));
    memcpy(ptr, se->l, size);
}

void strip_l(char** l) {
    while ((*l)[0] == SPACE) { (*l)++; }
}

void strip_r(char** r) {
    while ((*r)[0] == SPACE && (*r)[-1] == SPACE) { (*r)--; }
}

void strip(char** l, char** r) {
    strip_l(l);
    strip_r(r);
}

void strip_se(struct string_ends* se) {
    strip(&se->l, &se->r);
}

enum bool {
    true = 1,
    false = 0
};

enum bool is_quote(const char* it) {
    return *it == QUOTE_1 || *it == QUOTE_2;
}

enum bool is_space_delim(const char* it) {
    return *it == SPACE || *it == LINE_BREAK;
}

enum bool is_cmd_delim(const char* it) {
    return *it == BAR || *it == '>' || (*it == '>' && it[1] == '>') || *it == ';';
}


int parse_args(struct string_ends* args_string, struct string_ends** args_list_out) {
    struct string_ends* args_list = malloc(1 * sizeof(struct string_ends));
    *args_list_out = args_list;
    int args_cnt = 0;

    char* iter = args_string->l;
    char* iter_save = iter;

    int in_quotes = 0;

    while (iter <= args_string->r) {
        if (is_quote(iter)) { // "
            in_quotes = !in_quotes;
        }
        if (!in_quotes && is_space_delim(iter)) { // ' ', '\n'
            args_list[args_cnt].l = iter_save;
            args_list[args_cnt].r = iter;
            args_cnt++;

            void * new_ptr = realloc(args_list, (args_cnt + 1) * sizeof(struct string_ends));
            if (new_ptr != NULL) { args_list = new_ptr; }
            strip_l(&iter);
            iter_save = iter;
        }
        iter++;
    }

//    return args_list;
    return args_cnt;
}

struct cmd parse_exec(struct string_ends* exec_string) {
    struct cmd res_obj = (struct cmd) { .name = NULL, .argc = 0, .argv = malloc(0) };

    char* iter = exec_string->l;
    char* iter_save = iter;
//    strip_l(&iter_save);
    while (iter < exec_string->r) {
        if (is_space_delim(iter)) { break; }
        iter++;
    }

    res_obj.name = (struct string_ends) { .l = iter_save, .r = iter };
//    se_cpy( &(struct string_ends) { .l = exec_string->l, .r = iter }, res_obj.name);

    strip_l(&iter); // Gap between 'name' and 'args123...'

    struct string_ends* args_string = malloc(sizeof(struct string_ends));
    *args_string = (struct string_ends) { .l = iter, .r = exec_string->r };
    strip_se(args_string);

//    iter_save = iter;

//    struct string_ends* args_list_out;
    int argc = parse_args(args_string, &res_obj.argv);
//    int argc = sizeof(args_list_out) / sizeof(struct string_ends);

    res_obj.argc = argc;
//    res_obj.argv = args_list_out;

    return res_obj;

//    free(args_string); // TODO

//    while (iter != exec_string->r) {
//        pargs_arg
//    }
//    int prev_space = 0;
//    while (iter != exec_string->r) {
////        if (*iter == BAR) {
////            break;
////        }
//        if ((*iter == SPACE || *iter == '\n') && !prev_space) {
//            prev_space = 1;
//            strip(&iter_save, &iter);
//
////            printf("%d\n", (res_obj.argc + 1) * sizeof(struct string_ends));
//            void * new_ptr = realloc(res_obj.argv, (res_obj.argc + 1) * sizeof(struct string_ends));
//            if (new_ptr != NULL) { res_obj.argv = new_ptr; }
//
//            res_obj.argv[res_obj.argc].l = iter_save + 1;
//            res_obj.argv[res_obj.argc].r = iter - 1;
//            iter_save = iter + 1;
//            res_obj.argc++;
//        } else {
////            if (prev_space) {
////                iter_save++;
////            }
//            if (!(*iter == SPACE || *iter == '\n')) {
//                prev_space = 0;
//            }
//        }
//        iter++;
//    }

    return res_obj;
}

struct cmd_list {
    struct cmd* start;
    int size;
};


struct cmd_list parse_line(char* s) {
    unsigned s_size = strlen(s);

    struct cmd* result_commands = malloc(sizeof(struct cmd));
    int cmd_cnt = 0;

    char* it = s;
    char* iter_save = it;

    int in_quotes = 0;

    while (*it) {
        if (is_quote(it)) { in_quotes = !in_quotes; }
        if (is_cmd_delim(it) && !in_quotes) {
            struct string_ends* exec_string = malloc(sizeof(struct string_ends));
            *exec_string = (struct string_ends) { .l = iter_save, .r = it };
            *it = ' ';
            iter_save = it;

            strip_se(exec_string);

            void * tmp_ptr = realloc(result_commands, (cmd_cnt + 1) * sizeof(struct cmd));
            if (tmp_ptr != NULL) { result_commands = tmp_ptr; }

            result_commands[cmd_cnt] = parse_exec(exec_string);
            cmd_cnt++;
        }
        it++;
    }

    return (struct cmd_list) { .start = result_commands, .size = cmd_cnt };
}

void print_se(struct string_ends* se) {
    char* it = se->l;
    while (it < se->r) {
        printf("%c", *it);
        it++;
    }
}

void print_cmd(struct cmd c) {
    printf("--- CMD ---\n");
    printf("NAME: "); print_se(&c.name);
    printf("\nARGC: %d\n", c.argc);
    for (int i = 0; i < c.argc; ++i) {
        printf("ARG {%d}: \"", i);
        print_se(&c.argv[i]);
        printf("\"\n");
    }
    printf("--- END CMD ---\n");
}

void print_cmds(struct cmd_list cl) {
    for (int i = 0; i < cl.size; ++i) {
        printf("CMD {%d}:\n", i);
        print_cmd(cl.start[i]);
        printf("\n");
    }
}

//int read_string(char** s_in) {
//    int chunk_size = 10;
//    *s_in = malloc(chunk_size);
//    int s_size = 0;
//    int size_read;
//    do {
//        char buff[chunk_size];
//        size_read = fread(buff, 1, chunk_size, stdin);
//        strcpy(*s_in + s_size, buff);
//        s_size += size_read;
//
//        void* tmp_ptr = realloc(*s_in, s_size * sizeof(char));
//        if (tmp_ptr != NULL) { s_in = tmp_ptr; }
//    } while (size_read > 0);
//
//    return s_size;
//}

char* read_line() {
    int s_size = 0;
    char *raw_line = calloc(s_size, sizeof(char));
    char it, prev = 0;

    enum bool in_quotes = false;
    int i = 0;
    for (;;)
    {
        it = getc(stdin);
        if (is_quote(&it)) { in_quotes = !in_quotes; }
        if (!in_quotes && prev == BACKSLASH && it == LINE_BREAK) {
            prev = it;
            it = getc(stdin);


            raw_line[i - 1] = it;
            if (it == EOF || it == LINE_BREAK) {
                raw_line[i - 1] = ' ';
                break;
            }

            continue;
        }
        else if (it == EOF) break;
        else if (it == LINE_BREAK) break;

        s_size++;
        void* tmp_ptr = realloc(raw_line, sizeof(char) * s_size);
        if (tmp_ptr != NULL) { raw_line = tmp_ptr; }

        raw_line[i] = it;
        prev = it;
        i++;
    }

    raw_line = realloc(raw_line, sizeof(char) * s_size);
    raw_line[i] = ';';
    i++;
    raw_line[i] = '\0';

    return raw_line;
}

int main(int argc, char **argv) {
//    char s_in[1024];
////    scanf("%s", s_in);
//    fgets(s_in, 1024, stdin);
//
//    fgetc(stdin);

//    char* s_in;
//    read_string(&s_in);

    char* s_in = read_line();
    printf("string: \'%s\'\n", s_in);
    printf("size: %lu\n", strlen(s_in));

    struct cmd_list c_list = parse_line(s_in);
    print_cmds(c_list);
}

