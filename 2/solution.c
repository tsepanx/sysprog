
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define SPACE ' '
#define BAR '|'
#define LINE_BREAK '\n'
#define QUOTE_1 '\''
#define QUOTE_2 '\"'
#define BACKSLASH '\\'
#define SHARP '#'

enum bool {
    true = 1,
    false = 0
};

struct string_ends {
    char* l;
    char* r; // Right after last element
};

enum cmd_out_redir {
    STDOUT = 0,
    PIPE = 1,
    FILE_WRT = 2,
    FILE_APP = 3
};

//    struct string_ends name;
struct cmd {
    char* name;
    char** argv;
    int argc;
    enum cmd_out_redir redir_sign;
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

    char* dest = malloc((size + 1) * sizeof(char));
    memcpy(dest, se.l, size);
    dest[size] = '\0';

    return dest;
}

void strip_l(char** l, char ch) {
    while ((*l)[0] == ch) { (*l)++; }
}

void strip_r(char** r, char ch) {
    while ((*r)[0] == ch && (*r)[-1] == ch) { (*r)--; }
}

void strip(char** l, char** r, char ch) {
    strip_l(l, ch);
    strip_r(r, ch);
}

void strip_se_ch(struct string_ends* se, char ch) {
    strip(&se->l, &se->r, ch);
}

void strip_se_space(struct string_ends* se) {
    strip_se_ch(se, SPACE);
}

enum bool is_char(const char* it, const char ch) {
    int prev_not_backslash = it[-1] != BACKSLASH;
    return prev_not_backslash && (*it == ch);
}

enum bool is_quote(const char* it) {
    return is_char(it, QUOTE_1) || is_char(it, QUOTE_2);
}

enum bool is_space_delim(const char* it) {
    return is_char(it, SPACE) || is_char(it, LINE_BREAK);
}

enum bool is_cmd_delim(const char* it) {
    int prev_not_backslash = it[-1] != BACKSLASH;
    return prev_not_backslash && (*it == BAR || *it == '>' || (*it == '>' && it[1] == '>') || *it == ';');
}

enum cmd_out_redir delim_type(const char* it) {
    if (*it == BAR) {
        return PIPE;
    } else if (*it == '>') {
        return FILE_WRT;
    } else if (*it == '>' && it[1] == '>') {
        return FILE_APP;
    } else if (*it == ';') {
        return STDOUT;
    }
    return -1;
}


int parse_args(struct string_ends* args_string, char*** args_list_out) {
    char** args_list = malloc(2 * sizeof(char*));

    args_list[0] = malloc(2);
    strcpy(args_list[0], " ");

    int args_cnt = 1;

    if (se_size(args_string) == 0) {
        args_list[args_cnt] = NULL;
        *args_list_out = args_list;
        return args_cnt;
    }

    char* iter = args_string->l;
    char* iter_save = iter;

    int in_quotes1 = 0;
    int in_quotes2 = 0;

    while (iter <= args_string->r) {
        int is_already_in_quotes = in_quotes1 || in_quotes2;

        int is_any_delim = is_space_delim(iter) || is_cmd_delim(iter) || is_quote(iter);

        int end_of_quote1_arg = in_quotes1 && is_char(iter, QUOTE_1);
        int end_of_quote2_arg = in_quotes2 && is_char(iter, QUOTE_2);
        int end_of_simple_arg = !is_already_in_quotes && is_any_delim;

        int is_not_empty = iter > iter_save;
        if ((end_of_quote1_arg || end_of_quote2_arg || end_of_simple_arg) && is_not_empty) {
            struct string_ends se_arg = (struct string_ends) {.l = iter_save, .r = iter};

            if (is_char(iter, QUOTE_1)) {
                strip_se_ch(&se_arg, QUOTE_1);
                iter++;
                in_quotes1 = 0;
            } else if (is_char(iter, QUOTE_2)) {
                strip_se_ch(&se_arg, QUOTE_2);
                iter++;
                in_quotes2 = 0;
            }
            args_list[args_cnt] = se_dup(se_arg );
            args_cnt++;

            void * new_ptr = realloc(args_list, (args_cnt + 1) * sizeof(char *));
            if (new_ptr != NULL) { args_list = new_ptr; }

            strip_l(&iter, SPACE);
            iter_save = iter;
        }
        if (is_char(iter, QUOTE_1) && !in_quotes2) {
            in_quotes1 = !in_quotes1;
        }
        if (is_char(iter, QUOTE_2)  && !in_quotes1) {
            in_quotes2 = !in_quotes2;
        }
        iter++;
    }

    args_list[args_cnt] = NULL;
    *args_list_out = args_list; // TODO WHY?
    return args_cnt;
}

struct cmd parse_exec(struct string_ends* exec_string) {
    struct cmd res_obj = (struct cmd) { .name = NULL, .argc = 0, .argv = NULL, .redir_sign = -100 };

    char* iter = exec_string->l;
    char* iter_save = iter;
    while (iter < exec_string->r) {
        int is_delim = is_space_delim(iter);
        if (is_delim) { break; }
        iter++;
    }

    res_obj.name = se_dup((struct string_ends) { .l = iter_save, .r = iter });

    strip_l(&iter, SPACE); // Gap between 'name' and 'args123...'

    struct string_ends args_string = (struct string_ends) { .l = iter, .r = exec_string->r };
    strip_se_space(&args_string);

    int argc = parse_args(&args_string, &res_obj.argv);
    res_obj.argc = argc;

    return res_obj;
}

struct cmd_list parse_line(char* s) {
    struct cmd* result_commands = malloc(sizeof(struct cmd));
    int cmd_cnt = 0;

    char* it = s;
    char* iter_save = it;

    int in_quotes1 = 0;
    int in_quotes2 = 0;

    while (*it) {
        int in_quotes = in_quotes1 || in_quotes2;
        int not_empty = iter_save < it;

        if (is_cmd_delim(it) && !in_quotes && not_empty) {
            enum cmd_out_redir redir_sign = delim_type(it);
            struct string_ends exec_string = (struct string_ends) { .l = iter_save, .r = it };
            iter_save = it + 1;
            it++;

            strip_se_space(&exec_string);

            void * tmp_ptr = realloc(result_commands, (cmd_cnt + 1) * sizeof(struct cmd));
            if (tmp_ptr != NULL) { result_commands = tmp_ptr; }

            result_commands[cmd_cnt] = parse_exec(&exec_string);
            result_commands[cmd_cnt].redir_sign = redir_sign;
            cmd_cnt++;
        }

        if (is_char(it, QUOTE_1) && !in_quotes2) {
            in_quotes1 = !in_quotes1;
        }
        if (is_char(it, QUOTE_2)  && !in_quotes1) {
            in_quotes2 = !in_quotes2;
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
    printf("NAME: ");
//    print_se(&c.name);
    printf("%s", c.name);
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

char* read_line(FILE* stream, enum bool* is_eof) {
    int s_size = 0;
    char *raw_line = calloc(s_size, sizeof(char));
    char it, prev = 0;

    enum bool in_quotes = false;
    int i = 0;
    while (true) {
        it = getc(stream);
        if (is_quote(&it)) { in_quotes = !in_quotes; }
        if (!in_quotes && prev == BACKSLASH && it == LINE_BREAK) {
            prev = it;
            it = getc(stream);


            raw_line[i - 1] = it;
            if (it == LINE_BREAK) {
                raw_line[i - 1] = ' ';
                break;
            }

            continue;
        }
        else if (is_char(&it, SHARP)) break;
        else if (it == LINE_BREAK) break;
        else if (it == EOF) {
            *is_eof = true;
            break;
        }

        s_size++;
        void* tmp_ptr = realloc(raw_line, sizeof(char) * s_size);
        if (tmp_ptr != NULL) { raw_line = tmp_ptr; }

        raw_line[i] = it;
        prev = it;
        i++;
    }

    void* tmp_ptr = realloc(raw_line, sizeof(char) * (s_size + 1));
    if (tmp_ptr != NULL) { raw_line = tmp_ptr; }
    raw_line[i] = ';';
    raw_line[i + 1] = '\0';

    return raw_line;
}

void free_cmds(struct cmd_list* cmds) {
    for (int i = 0; i < cmds->size; ++i) {
        struct cmd* a = &cmds->start[i];
        for (int j = 0; j < a->argc; ++j) {
            free(a->argv[j]);
        }
        free(a->name);
        free(a->argv);
    }
    free(cmds->start);
}

int process_pipes(struct cmd_list c_list) {
    int pids[c_list.size];

    int pipe_start[2];
    int pipe_end[2];

    if (c_list.size > 1)
    {
        pipe(pipe_start);
    }

    for (int i = 0; i < c_list.size; ++i) {
        if (i < c_list.size - 1) {
            pipe(pipe_end);
        }

        struct cmd cmd_cur = c_list.start[i];

        if (strcmp(cmd_cur.name, "cd") == 0) {
            int cd_result = chdir(cmd_cur.argv[1]);
            if (cd_result == -1) {
                printf("cd: no dir found: %s\n", cmd_cur.argv[1]);
            }
            continue;
        }
        else if (strcmp(cmd_cur.name, "exit") == 0 && c_list.size == 1) {
            int code;
            if (cmd_cur.argc > 1) {
                code = atol(cmd_cur.argv[1]);
            }
            else {
                code = 0;
            }
            exit(code);
        }

        pids[i] = fork();

        if (pids[i] < 0) {
            fprintf(stderr, "FORK() BROKEN\n");
            return -1;
        }
        if (pids[i] == 0) { // CHILD
            if (i > 0) {
                dup2(pipe_start[0], STDIN_FILENO);
                close(pipe_start[0]);
                close(pipe_start[1]);
            }

            if (i < c_list.size - 1) {
                close(pipe_end[0]);
                dup2(pipe_end[1], STDOUT_FILENO);
                close(pipe_end[1]);
            }

//            struct cmd cmd_cur = c_list.start[i];

            execvp(cmd_cur.name, cmd_cur.argv);
            fprintf(stderr, "my-shell: %s: %s\n", cmd_cur.name, strerror(errno));
            return errno;
        } else { // PARENT
            if (i > 0) {
                close(pipe_start[0]);
                close(pipe_start[1]);
            }

            if (i < c_list.size - 1) {
                pipe_start[0] = pipe_end[0];
                pipe_start[1] = pipe_end[1];
            }
        }

    }

    if (c_list.size > 1) {
        close(pipe_start[0]);
        close(pipe_start[1]);
    } else {
        return 0;
    }

    int res_status;
    for (int i = 0; i < c_list.size; i++) {
        int status;
        waitpid(pids[i], &status, 0);

        if (WIFEXITED(status)) {
            res_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            res_status = EXIT_FAILURE;
        }
    }

    return res_status;
    // TODO
    // ~~TODO 1) Add cmd_out_redir~~
    // ~~TODO 2) Reidrect from one child to next (by chain)~~
    // TODO 3) Ability to write to files
}


int main() {
    while (true) {
        enum bool is_eof = false;

        char* s_in = read_line(stdin, &is_eof);

        if (is_eof) {
            free(s_in);
            break;
        }

//        printf("string: \'%s\'\n", s_in);
        struct cmd_list c_list = parse_line(s_in);
//        print_cmds(c_list);

        int result = process_pipes(c_list);

        free_cmds(&c_list);
        free(s_in);
    }

    fclose(stdin);
}

