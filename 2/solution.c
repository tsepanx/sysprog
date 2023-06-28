
#include <string.h>
#include <malloc.h>

#define SPACE ' '
#define BAR '|'

//struct cmd {
//    char *name;
//    const char **argv;
//    int argc;
//};

struct string_ends {
    char* l;
    char* r;
};

struct cmd2 {
    struct string_ends name;
    struct string_ends* argv;
    int argc;
};

int se_size(struct string_ends* se) {
    return (int) (se->r - se->l + 1);
}

void se_cpy(struct string_ends* se, char* ptr) {
    // MALLOC
    int size = se_size(se);

    ptr = malloc(size * sizeof(char *));
    memcpy(ptr, se->l, size);
}

void strip_l(char** l) {
    while (*l[0] == SPACE) { (*l)++; }
    if ((*l)[-1] == SPACE) {
        (*l)--;
    }
}

void strip_r(char** r) {
    while (*r[0] == SPACE) { (*r)--; }
    if ((*r)[1] == SPACE) { (*r)++; }
}

void strip(char** l, char** r) {
    strip_l(l);
    strip_r(r);
}

struct cmd2 parse_cmd(struct string_ends* se) {
//    int s_size = se_size(se);
    struct cmd2 res_cmd = (struct cmd2) { .name = NULL, .argc = 0, .argv = malloc(0) };

    char* it = se->l;
    char* it_l = it;
    strip_l(&it_l);
    while (it != se->r) {
        if (*it == SPACE) { break; }
        it++;
    }

    res_cmd.name = (struct string_ends) { .l = se->l, .r=(it - 1) };
//    se_cpy( &(struct string_ends) { .l = se->l, .r = it }, res_cmd.name);

    it++;
    it_l = it;
    int prev_space = 0;
    while (it != se->r) {
//        if (*it == BAR) {
//            break;
//        }
        if ((*it == SPACE || *it == '\n') && !prev_space) {
            prev_space = 1;
            strip(&it_l, &it);

//            printf("%d\n", (res_cmd.argc + 1) * sizeof(struct string_ends));
            void * new_ptr = realloc(res_cmd.argv, (res_cmd.argc + 1) * sizeof(struct string_ends));
            if (new_ptr != NULL) { res_cmd.argv = new_ptr; }

            res_cmd.argv[res_cmd.argc].l = it_l + 1;
            res_cmd.argv[res_cmd.argc].r = it - 1;
            it_l = it + 1;
            res_cmd.argc++;
        } else {
//            if (prev_space) {
//                it_l++;
//            }
            if (!(*it == SPACE || *it == '\n')) {
                prev_space = 0;
            }
        }
        it++;
    }

    return res_cmd;
}

struct cmd_list {
    struct cmd2* start;
    int size;
};


struct cmd_list parse_line(char* s) {
    unsigned s_size = strlen(s);

    struct cmd2* res_cmd_list = malloc(sizeof(struct cmd2));
    int cmd_cnt = 0;

    char* it_l = s;
    for (int i = 0; i < s_size; ++i) {
        if (s[i] == BAR || s[i] == '\n') {

            struct string_ends* cmd_se = malloc(sizeof(struct string_ends));
            cmd_se->l = it_l;
            cmd_se->r = s + i;

            it_l += i + 1;


            strip(&cmd_se->l, &cmd_se->r);

            void * tmp_ptr = realloc(res_cmd_list, (cmd_cnt + 1) * sizeof(struct cmd2));
            if (tmp_ptr != NULL) { res_cmd_list = tmp_ptr; }

            res_cmd_list[cmd_cnt] = parse_cmd(cmd_se);
            cmd_cnt++;
        }
    }

    return (struct cmd_list) { .start = res_cmd_list, .size = cmd_cnt };
}

void print_se(struct string_ends* se) {
    char* it = se->l;
    while (it != se->r) {
        printf("%c", *it);
        it++;
    }
    printf("%c", *it);
}

void print_cmd(struct cmd2 c) {
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

int main(int argc, char **argv) {
    char s_in[1024];
//    scanf("%s", s_in);
    fgets(s_in, 1024, stdin);

    printf("AAA\n");

    struct cmd_list c_list = parse_line(s_in);
    print_cmds(c_list);
}

