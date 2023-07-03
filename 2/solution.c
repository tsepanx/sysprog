
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#define SPACE ' '
#define BAR '|'
#define LINE_BREAK '\n'
#define QUOTE_1 '\''
#define QUOTE_2 '\"'
#define BACKSLASH '\\'

enum bool {
    true = 1,
    false = 0
};

struct string_ends {
    char* l;
    char* r; // Right after last element
};

//enum cmd_out_redir {
//    STDOUT = 0,
//    PIPE = 1,
//    FILE_WRT = 2,
//    FILE_APP = 3
//};

struct cmd {
    struct string_ends name;
//    struct string_ends* argv;
    char** argv;
    int argc;
//    enum cmd_out_redir;
};

struct cmd_list {
    struct cmd* start;
    int size;
};

int se_size(struct string_ends* se) {
    return (int) (se->r - se->l);
}

char* se_dup(struct string_ends se) {
    int size = se_size(&se);

    char* dest = calloc(size, sizeof(char));
    memcpy(dest, se.l, size);
    return dest;
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

enum bool is_quote(const char* it) {
    return *it == QUOTE_1 || *it == QUOTE_2;
}

enum bool is_space_delim(const char* it) {
    return *it == SPACE || *it == LINE_BREAK;
}

enum bool is_cmd_delim(const char* it) {
    return *it == BAR || *it == '>' || (*it == '>' && it[1] == '>') || *it == ';';
}


int parse_args(struct string_ends* args_string, char*** args_list_out) {
    char** args_list = malloc(1 * sizeof(char*));
//    struct string_ends* args_list = malloc(1 * sizeof(struct string_ends));
    int args_cnt = 0;

    char* iter = args_string->l;
    char* iter_save = iter;

    int in_quotes = 0;

    while (iter <= args_string->r) {
        if (is_quote(iter)) { // "
            in_quotes = !in_quotes;
        }
        if (!in_quotes && is_space_delim(iter)) { // ' ', '\n'
//            args_list[args_cnt].l = iter_save;
//            args_list[args_cnt].r = iter;
            args_list[args_cnt] = se_dup((struct string_ends) {.l = iter_save, .r = iter});
            args_cnt++;

//            void * new_ptr = realloc(args_list, (args_cnt + 1) * sizeof(struct string_ends));
            void * new_ptr = realloc(args_list, (args_cnt + 1) * sizeof(char *));
            if (new_ptr != NULL) { args_list = new_ptr; }

            strip_l(&iter);
            iter_save = iter;
        }
        iter++;
    }

    *args_list_out = args_list; // TODO WHY?
    return args_cnt;
}

struct cmd parse_exec(struct string_ends* exec_string) {
    struct cmd res_obj = (struct cmd) { .name = NULL, .argc = 0, .argv = NULL };

    char* iter = exec_string->l;
    char* iter_save = iter;
    while (iter < exec_string->r) {
        if (is_space_delim(iter)) { break; }
        iter++;
    }

    res_obj.name = (struct string_ends) { .l = iter_save, .r = iter };

    strip_l(&iter); // Gap between 'name' and 'args123...'

    struct string_ends args_string = (struct string_ends) { .l = iter, .r = exec_string->r };
    strip_se(&args_string);

    int argc = parse_args(&args_string, &res_obj.argv);

    res_obj.argc = argc;

    return res_obj;
}

struct cmd_list parse_line(char* s) {
    struct cmd* result_commands = malloc(sizeof(struct cmd));
    int cmd_cnt = 0;

    char* it = s;
    char* iter_save = it;

    int in_quotes = 0;

    while (*it) {
        if (is_quote(it)) { in_quotes = !in_quotes; }
        if (is_cmd_delim(it) && !in_quotes) {
            struct string_ends exec_string = (struct string_ends) { .l = iter_save, .r = it };
            *it = ' ';
            iter_save = it;

            strip_se(&exec_string);

            void * tmp_ptr = realloc(result_commands, (cmd_cnt + 1) * sizeof(struct cmd));
            if (tmp_ptr != NULL) { result_commands = tmp_ptr; }

            result_commands[cmd_cnt] = parse_exec(&exec_string);
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
//        print_se(&c.argv[i]);
        printf("%s", c.argv[i]);
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

char* read_line() {
    int s_size = 0;
    char *raw_line = malloc(s_size * sizeof(char));
    char it, prev = 0;

    enum bool in_quotes = false;
    int i = 0;
    while (true)
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
        else if (it == EOF || it == LINE_BREAK) break;

        s_size++;
        void* tmp_ptr = realloc(raw_line, sizeof(char) * s_size);
        if (tmp_ptr != NULL) { raw_line = tmp_ptr; }

        raw_line[i] = it;
        prev = it;
        i++;
    }

    raw_line[i] = ';';

    return raw_line;
}

void free_cmds(struct cmd_list* cmds) {
    for (int i = 0; i < cmds->size; ++i) {
        struct cmd* a = &cmds->start[i];
        for (int j = 0; j < a->argc; ++j) {
            free(a->argv[j]);
        }
        free(a->argv);
    }
    free(cmds->start);
}

int main() {
    char* s_in = read_line();

    printf("string: \'%s\'\n", s_in);

    struct cmd_list c_list = parse_line(s_in);
    print_cmds(c_list);

//    pid_t proc = fork();
//    if (proc < 0) {
//        fprintf(stderr, "Fork failed\n");
//    } else if (proc == 0) {
//        struct cmd cmd1 = c_list.start[0];
////        se_dup(cmd1.argv)
//    } else {
//
//    }

    // ----------

    free_cmds(&c_list);
    free(s_in);

    fclose(stdin);
}

