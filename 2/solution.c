
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>

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

enum cmd_out_redir {
    STDOUT = 0,
    PIPE = 1,
    FILE_WRT = 2,
    FILE_APP = 3
};

struct cmd {
//    struct string_ends name;
    char* name;
//    struct string_ends* argv;
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
//    struct string_ends* args_list = malloc(1 * sizeof(struct string_ends));

    args_list[0] = malloc(1); *args_list[0] = ' ';
    int args_cnt = 1;

    char* iter = args_string->l;
    char* iter_save = iter;

    int in_quotes = 0;

    while (iter <= args_string->r) {
        if (!in_quotes && (is_space_delim(iter) || is_cmd_delim(iter) || is_quote(iter)) && (iter > iter_save)) { // ' ', '\n'
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
        if (is_quote(iter)) { // "
            in_quotes = !in_quotes;
        }
        iter++;
    }

    *args_list_out = args_list; // TODO WHY?
    return args_cnt;
}

struct cmd parse_exec(struct string_ends* exec_string) {
    struct cmd res_obj = (struct cmd) { .name = NULL, .argc = 0, .argv = NULL, .redir_sign = -100 };

    char* iter = exec_string->l;
    char* iter_save = iter;
    while (iter < exec_string->r) {
        if (is_space_delim(iter)) { break; }
        iter++;
    }

    res_obj.name = se_dup((struct string_ends) { .l = iter_save, .r = iter });

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
            enum cmd_out_redir redir_sign = delim_type(it);
            struct string_ends exec_string = (struct string_ends) { .l = iter_save, .r = it };
//            it++;
//            *it = ' ';
            iter_save = it + 1;
            it++;

            strip_se(&exec_string);

            void * tmp_ptr = realloc(result_commands, (cmd_cnt + 1) * sizeof(struct cmd));
            if (tmp_ptr != NULL) { result_commands = tmp_ptr; }

            result_commands[cmd_cnt] = parse_exec(&exec_string);
            result_commands[cmd_cnt].redir_sign = redir_sign;
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

char* read_line(FILE* stream) {
    int s_size = 0;
    char *raw_line = malloc(s_size * sizeof(char));
    char it, prev = 0;

    enum bool in_quotes = false;
    int i = 0;
    while (true)
    {
        it = getc(stream);
        if (is_quote(&it)) { in_quotes = !in_quotes; }
        if (!in_quotes && prev == BACKSLASH && it == LINE_BREAK) {
            prev = it;
            it = getc(stream);


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
        free(a->name);
        free(a->argv);
    }
    free(cmds->start);
}

int post_process1(struct cmd_list c_list) {
//    int prev_pipe[2];
    int prev_out_fd = 0;
    int cur_pipe[2];
    int children[c_list.size];

    for (int i = 0; i < c_list.size; ++i) {
//        printf("%s: %d\n", c_list.start[i].name, c_list.start[i].redir_sign);

//        int procs_fd[c_list.size];

//        int prev_out_fd = 0;

//        if (i > 0) {
//            prev_pipe[0] = cur_pipe[0];
//            prev_pipe[1] = cur_pipe[1];
//        }
        pipe(cur_pipe);

//        pipe(prev_pipe);
//        pipe(cur_pipe);

        pid_t proc = fork();
        if (proc < 0) {
            fprintf(stderr, "Fork failed\n");
        } else if (proc == 0) { // CHILD
            close(cur_pipe[0]);
            if (i < c_list.size - 1) {
                if (c_list.start[i].redir_sign != STDOUT) {
                    dup2(cur_pipe[1], STDOUT_FILENO);
                }
            }

            if (i > 0) {
                if (prev_out_fd != 0) {
                    dup2(prev_out_fd, STDIN_FILENO);
                }
//                } else {
//                    fprintf(stderr, "%d: Wrong prev_pipe[0]\n", i);
//                }
            }

            struct cmd cmd_cur = c_list.start[i];
            fprintf(stderr, "NAME: %s\n", cmd_cur.name);
            execvp(cmd_cur.name, cmd_cur.argv);
            fprintf(stderr, "my-shell: %s: %s\n", strerror(errno), cmd_cur.name);

            return -1;
        } else { // PARENT
            children[i] = proc;

            close(cur_pipe[1]);

            if (i > 0) {

            }

            if (i < c_list.size - 1) {
                if (c_list.start[i].redir_sign == PIPE) {
                    prev_out_fd = cur_pipe[0];
                } else if (c_list.start[i].redir_sign == STDOUT) {
                    prev_out_fd = 0;
                    printf("TO STDOUT: %d", cur_pipe[0]);
//                    dup2(cur_pipe[0], STDOUT_FILENO);
                } else if (c_list.start[i].redir_sign == FILE_WRT) {
                    int fd_fout = open(c_list.start[i + 1].name, O_WRONLY);
                    dup2(cur_pipe[0], fd_fout);
                }
                printf("%d: %d\n", i, prev_out_fd);
            }
//                wait(NULL);

//                int out_fd = STDOUT_FILENO;
//                char buff[128];
//                size_t count = read(cur_pipe[0], buff, sizeof(buff));
//
//                while (count > 0) {
//                    write(out_fd, buff, count);
//                    count = read(cur_pipe[0], buff, sizeof(buff));
//                }
        }
    }

    int code;
    int final_exit_code;
    for (int i = 0; i < c_list.size; ++i) {
        assert(waitpid(children[i], &code, 0) >= 0);
        printf("code: %d\n", code);

        if (WIFEXITED(code)) {
            final_exit_code &= WEXITSTATUS(code);
        } else if (WIFSIGNALED(code)) {
            final_exit_code = EXIT_FAILURE;
        }
    }

    return final_exit_code;

    // TODO
    // ~~TODO 1) Add cmd_out_redir~~
    // TODO 2) Reidrect from one child to next (by chain)
    // TODO 3) Ability to write to files


}

int post_process(struct cmd_list c_list) {
    int children[c_list.size];

    int parent_write[2];
    int child_write[2];
    pipe(parent_write);
    pipe(child_write);

    pid_t proc = fork();

//    int i = 0;
    for (int i = 0; i < c_list.size; ++i) {

        if (proc < 0) {
            fprintf(stderr, "FORK() BROKEN\n");
            return -1;
        }
        if (proc == 0) { // CHILD
//        printf("CHILD\n");

//        close(parent_write[1]);
//        close(child_write[0]);
//
//        dup2(parent_write[0], STDIN_FILENO);
//        dup2(child_write[1], STDOUT_FILENO);
//
//        close(parent_write[0]);
//        close(child_write[1]);

            if (i > 0) {
                dup2(pipe_start[0], STDIN_FILENO);
                close(pipe_start[0]);
                close(pipe_start[1]);
            }

            if (i != c_list.size - 1) {
                close(pipe_end[0]);
                dup2(pipe_end[1], STDOUT_FILENO);
                close(pipe_end[1]);
            }

            struct cmd cmd_cur = c_list.start[i];

            execvp(cmd_cur.name, cmd_cur.argv);
            fprintf(stderr, "my-shell: %s: %s\n", strerror(errno), cmd_cur.name);
            return errno;
        } else { // PARENT
            printf("PARENT: %d\n", proc);

            close(parent_write[0]);
            close(child_write[1]);

//        dup2(parent_write[1], STDIN_FILENO);
//        dup2(parent_write[1], STDOUT_FILENO);

            int prev_proc_out_fd = child_write[0];

//        char* parent_input = read_line(stdin);

//        write(parent_write[1], parent_input, sizeof(parent_input));
//        free(parent_input);

//        wait(NULL);

            char buf[128];
            unsigned count;
            while ((count = read(prev_proc_out_fd, buf, sizeof(buf))) > 0) {
                fprintf(stderr, "CHUNK (size %d): %s\n---\n", count, buf);
            }
        }

    }

    return 0;
    // TODO
    // ~~TODO 1) Add cmd_out_redir~~
    // TODO 2) Reidrect from one child to next (by chain)
    // TODO 3) Ability to write to files


}


int main() {
    char* s_in = read_line(stdin);

    printf("string: \'%s\'\n", s_in);

    struct cmd_list c_list = parse_line(s_in);
    print_cmds(c_list);

    int result = post_process(c_list);

    free_cmds(&c_list);
    free(s_in);

    fclose(stdin);

    return result;
}

