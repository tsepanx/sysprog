
//#ifndef AAA
//#define AAA

#include "userfs.h"
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <bits/time.h>
#include <time.h>
#include <unistd.h>

#include "my.c"

enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	/** Block memory. */
//	char *memory;
	/** How many bytes are occupied. */
	int occupied;
	/** Next block in the file. */
	struct block *next;
	/** Previous block in the file. */
	struct block *prev;

	/* PUT HERE OTHER MEMBERS */
    int i;
    char** file_memory_ptr;
};

struct file {
	/** Double-linked list of file blocks. */
	struct block *block_list;
	/**
	 * Last block in the list above for fast access to the end
	 * of file.
	 */
	struct block *last_block;
	/** How many file descriptors are opened on the file. */
	int refs;
	/** File name. */
	char *name;
	/** Files are stored in a double-linked list. */
	struct file *next;
	struct file *prev;

	/* PUT HERE OTHER MEMBERS */
    int blocks_count;
    char* memory_ptr;
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
	struct file *file;

    int ptr_block_i;
    int ptr_block_offset;

    int deleted;

	/* PUT HERE OTHER MEMBERS */
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;

char* get_memory(struct block* b) {
    char* res = *(b->file_memory_ptr) + (b->i * BLOCK_SIZE);
    return res;
}

void print_block(struct block* b) {
    printf("    -BLOCK-: %p\n", b);
    printf("        %p <-prev next-> %p\n", b->prev, b->next);
    printf("        MEM: %d, '%.10s'\n", b->occupied, get_memory(b));
}

void print_file(struct file* f) {
    printf("-FILE-: %s\n", f->name);

    struct block* bi = f->block_list;
    while (1) {
        print_block(bi);

        if (bi->next == NULL) {
            break;
        }
        bi = bi->next;
    }

    printf("REFS: %d\n", f->refs);
}

void print_fd(struct filedesc* fd) {
    if (fd == NULL) {
        printf("-FD-: NULL\n");
        return;
    }
    assert(fd->file != NULL);

    printf("-FD-: -> '%s'\n", fd->file->name);
    printf("    I, OFFSET: %d, %d\n", fd->ptr_block_i, fd->ptr_block_offset);
}

void print_debug() {
    printf("============================================DEBUG===============================================\n");
    printf("__FD LIST__\n");
    for (int i = 0; i < file_descriptor_capacity; ++i) {
        printf("%d: ", i);
        print_fd(file_descriptors[i]);
    }

    printf("\n\n\n__FILES__\n");
    struct file* f = file_list;
    while (f != NULL) {
        print_file(f);
        f = f->next;
    }
    printf("============================================ ||| ===============================================\n");
}

struct block* init_block(struct block* prev, struct block* next, char** file_memory_ptr, int i) {
    struct block* b = malloc(sizeof(struct block));
//    b->memory = calloc(BLOCK_SIZE, sizeof(char));
//    b->memory = file_memory_ptr;
    b->file_memory_ptr = file_memory_ptr;
    b->occupied = 0;
    b->next = next;
    b->prev = prev;
    b->i = i;

    return b;
}

struct file* init_file(const char* filename, struct file* prev, struct file* next) {
    struct file* f = malloc(sizeof(struct file));
    f->name = calloc(strlen(filename), sizeof(char));
    strcpy(f->name, filename);

    f->blocks_count = 1;
    f->memory_ptr = malloc(1 * BLOCK_SIZE * sizeof(char));

    struct block* b = init_block(NULL, NULL, &f->memory_ptr, 0);
    f->block_list = b;
    f->last_block = b;
    f->prev = prev;
    f->next = next;

    f->refs = 1;

    return f;
}

struct filedesc* init_fd(struct file* f) {
    struct filedesc* fd = malloc(sizeof(struct filedesc));
    fd->file = f;
    fd->ptr_block_i = 0;
    fd->ptr_block_offset = 0;
    fd->deleted = 0;

    return fd;
}

int get_size(struct file* f) {
    assert(f->block_list != NULL);

    int result_size = 0;

    struct block* bi = f->block_list;
    while (bi != NULL) {
        if (bi->next != NULL) {
            assert(bi->occupied == BLOCK_SIZE);
        }

        result_size += bi->occupied;
        bi = bi->next;
    }
    return result_size;
}

void free_block(struct block* b) {
//    free(b->memory);
    free(b);
}

void free_file(struct file* f) {
    struct block* bi = f->block_list;

    int i = 0;

    double avg_blocks = (double) get_size(f) / BLOCK_SIZE;

    printf("OVERALL BLOCKS: %f\n", avg_blocks);

    while (bi != NULL) {
        if (i % 100 == 0) {
            printf("FREEING BLOCK: %d\n", i);
        }
        struct block* bn = bi->next;
        free_block(bi);

        bi = bn;
        i++;
    }
    free(f->name);
    free(f->memory_ptr);
    free(f);
}

void free_file_desc(struct filedesc* fd) {
    assert(fd != NULL);
//    assert(*fd != NULL);
//    fd->file = NULL;
//    fd->ptr_block_offset = -1;
//    fd->ptr_block_i = -1;
//    fd->deleted = 1;
    free(fd);
}

void add_block(struct file* f) {
    f->memory_ptr = realloc(f->memory_ptr, (f->blocks_count + 1) * BLOCK_SIZE);
//    char* new_mem_ptr = f->memory_ptr + (f->blocks_count * BLOCK_SIZE);

    struct block* b = init_block(f->last_block, NULL, &f->memory_ptr, f->blocks_count);
    f->blocks_count++;

    f->last_block->next = b;
    f->last_block = b;
}

void add_file_to_list(struct file* f, struct file** flist_ptr) {
    struct file* file_list_obj = *flist_ptr;

    if (file_list_obj == NULL) {
        *flist_ptr = f;
        return;
    }

    while (1) {
        if (file_list_obj->next != NULL) {
            file_list_obj = file_list_obj->next;
        } else {
            break;
        }
    }

    file_list_obj->next = f;
    f->prev = file_list_obj;
}

int add_fd_to_list(struct filedesc* fd) {
    file_descriptor_count++;

    if (file_descriptor_capacity == 0) {
        assert(file_descriptors == NULL);

        file_descriptors = calloc(1, sizeof(struct filedesc*));
        file_descriptors[0] = fd;
    } else {
        int ii = 0;

        while (ii < file_descriptor_capacity) {
            struct filedesc* fdi = file_descriptors[ii];
            if (fdi == NULL) {
                file_descriptors[ii] = fd;
                return ii;
            }
            ii++;
        }

//        printf("fd_capacity: %d, %lu\n", file_descriptor_capacity, (file_descriptor_capacity + 1) * sizeof(struct filedesc*));
        file_descriptors = realloc(file_descriptors, (file_descriptor_capacity + 1) * sizeof(struct filedesc*));
        file_descriptors[file_descriptor_capacity] = fd;
    }

    return file_descriptor_capacity++;
}

void remove_file_from_list(struct file* f, struct file** f_list) {
    if (!f->next && !f->prev) {
        *f_list = NULL;
    }
    if (f->next && !f->prev) {
        f->next->prev = NULL;
    }
    if (!f->next && f->prev) {
        f->prev->next = NULL;
    }
    if (f->next && f->prev) {
        f->prev->next = f->next;
        f->next->prev = f->prev;
    }
}

void remove_fd_from_list_by_i(int i) {
    assert(file_descriptors != NULL);
    assert(file_descriptor_count > 0);
    assert(file_descriptor_capacity > 0);

    file_descriptors[i] = NULL;
    file_descriptor_count--;
}

unsigned long microsec(struct timespec ts) {
    return ts.tv_sec * (unsigned) 1e6 + ts.tv_nsec / (unsigned) 1e3;
}

unsigned long get_cur_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return microsec(ts);
}

void destroy_file(struct file* f) {
    f->name = realloc(f->name, strlen(f->name) + 100);

    sprintf(f->name, "%s__ghost_file_time_%lu", f->name, get_cur_time());
//    printf("NEW FNAME: %s\n", f->name);
//    remove_file_from_list(f, &file_list);
//    free_file(f); // ODO add to "ghost" files
}

struct filedesc* get_fd(int i) {
    if (i < 0 || i >= file_descriptor_capacity || file_descriptors[i] == NULL) {
        return NULL;
    }

    return file_descriptors[i];
}

int destroy_fd(int i) {
    struct filedesc* fd = get_fd(i);
    if (fd == NULL) {
        return -1;
    }

    assert(fd->file != NULL);
    fd->file->refs--;
    remove_fd_from_list_by_i(i);

    if (i == 0) {
        printf("DESTROY FD: 0, %p\n", fd);
    }

    free_file_desc(fd);

    return 0;
}

struct block* get_block_by_i(struct file* f, int target_i) {
    assert(f->block_list != NULL);

    struct block* bi = f->block_list;
    int i = 0;
    while (1) {
        if (i == target_i) {
            return bi;
        }

        if (bi->next == NULL) {
            break;
        }

        i++;
        bi = bi->next;
    }

    return NULL;
}

int write_to_fd(struct filedesc* fd, const char* buf, int size) {
    assert(fd != NULL);
    assert(fd->file != NULL);

    int result_written = 0;

    struct file* f = fd->file;
    int filesize = get_size(f);
    struct block* b_cur = get_block_by_i(f, fd->ptr_block_i);

//    printf("FILESIZE: %s, %d\n", f->name, filesize);

    const char* c = buf;
    while (result_written < size) {
//        assert(*c != '\0');
        // block* b_cur;
        // int block_offset;
//        fd->ptr_block_offset

        if ((filesize + 1) > MAX_FILE_SIZE) {
            ufs_error_code = UFS_ERR_NO_MEM;
            return -1;
        }

        assert(fd->ptr_block_offset <= BLOCK_SIZE); // CHECK FOR ptr NOT EXCEEDING BORDER

        if (fd->ptr_block_offset == BLOCK_SIZE) { // END OF CUR BLOCK
            if (b_cur->next == NULL) { // APPENDING NEW BLOCK
                add_block(f);

                assert(b_cur->next != NULL);
                b_cur = b_cur->next;
            }
            fd->ptr_block_i++;
            fd->ptr_block_offset = 0;
        }

        char* b_mem = get_memory(b_cur);

        b_mem[fd->ptr_block_offset] = *c; // WRITING CHAR
        result_written++;
        filesize++;

        fd->ptr_block_offset++;
        b_cur->occupied = fmax(b_cur->occupied, fd->ptr_block_offset);
//        printf("OCCUPIED CHANGE: %d\n", b_cur->occupied);
        c++;
//        print_debug();
    }

    return result_written;
}

int read_from_fd(struct filedesc* fd, char* out_buf, int n) {
    assert(fd != NULL);
    assert(fd->file != NULL);

    int result_read = 0;

    struct file* f = fd->file;
    struct block* b_cur = get_block_by_i(f, fd->ptr_block_i);

    while (result_read < n) { // NOT read REQUESTED COUNT
        assert(fd->ptr_block_offset <= BLOCK_SIZE); // CHECK FOR ptr NOT EXCEEDING BORDER on more than 1
//        assert(fd->ptr_block_offset <= b_cur->occupied);

        if (fd->ptr_block_offset == b_cur->occupied && b_cur->occupied != BLOCK_SIZE) { // REACHED 'EOF'
            break;
        }


        if (fd->ptr_block_offset == BLOCK_SIZE) { // END OF CUR BLOCK
            if (b_cur->next == NULL) { // THIS IS LAST BLOCK
                break;
            } else {
                b_cur = b_cur->next; // MOVING TO NEXT BLOCK
                fd->ptr_block_i++;
                fd->ptr_block_offset = 0;
            }
        }

        out_buf[result_read] = get_memory(b_cur)[fd->ptr_block_offset]; // READING CHAR
//        b_cur->memory[fd->ptr_block_offset] = *c; // WRITING CHAR
        result_read++;

        fd->ptr_block_offset++;
    }

    return result_read;
}

struct file* find_existing_filename(struct file *f_list, const char* filename) {
    struct file* fi = f_list;

    while (fi != NULL) {
        if (strcmp(fi->name, filename) == 0) {
            return fi;
        }
        fi = fi->next;
    }
    return NULL;
}

enum ufs_error_code
ufs_errno()
{
	return ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{
	/* IMPLEMENT THIS FUNCTION */
//	(void)filename;
//	(void)flags;
//	(void)file_list;
//	(void)file_descriptors;
//	(void)file_descriptor_count;
//	(void)file_descriptor_capacity;
//	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
//	return -1;

    printf("\n======== OPEN: %s, %d\n", filename, flags);
//    print_debug();

    //    static struct file *file_list = NULL;
    struct file *existing_file = find_existing_filename(file_list, filename);
    struct file *res_file;
    struct filedesc *res_fd;

    if (flags == UFS_CREATE) {
        if (existing_file == NULL) {
            res_file = init_file(filename, NULL, NULL);
            add_file_to_list(res_file, &file_list);
        } else {
            res_file = existing_file;
        }
    } else if (flags == 0) {                                        // OPEN EXISTING
        if (existing_file == NULL) {
            ufs_error_code = UFS_ERR_NO_FILE;
            return -1;
        } else {
            res_file = existing_file;
            res_file->refs++;
        }
    }

    res_fd = init_fd(res_file);
    int fd_i = add_fd_to_list(res_fd);
    return fd_i;
}

ssize_t
ufs_write(int i, const char *buf, size_t size)
{

    printf("\n======== WRITE: %d, %lu\n", i, size);

	/* IMPLEMENT THIS FUNCTION */
//	(void)fd;
//	(void)buf;
//	(void)size;
//	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
//	return -1;

    if (size > MAX_FILE_SIZE) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }

    struct filedesc* fd = get_fd(i);

    if (fd == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    if (buf == NULL) {
        return -1;
    }

    int res = write_to_fd(fd, buf, size);
//    print_debug();
    return res;
}

ssize_t
ufs_read(int i, char *buf, size_t size)
{
    printf("\n======== READ: %d, %.5s, %lu\n", i, buf, size);
//    print_debug();

	/* IMPLEMENT THIS FUNCTION */
//	(void)fd;
//	(void)buf;
//	(void)size;
//	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
//	return -1;

    struct filedesc* fd = get_fd(i);

    if (fd == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }


    int res = read_from_fd(fd, buf, size);
//    printf("RES READ: '%s', %d\n", buf, res);
    return res;
}

int
ufs_close(int fd)
{
//    printf("\n======== CLOSE: %d\n", fd);
	/* IMPLEMENT THIS FUNCTION */
//	(void)fd;

//	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
//	return -1;

    return destroy_fd(fd);
}

int
ufs_delete(const char *filename)
{
//    printf("\n======== DELETE: %s\n", filename);
	/* IMPLEMENT THIS FUNCTION */
//	(void)filename;
//	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
//	return -1;

    struct file *existing_file = find_existing_filename(file_list, filename);

    if (existing_file == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    } else {
        destroy_file(existing_file);
        return 0;
    }
}

void
ufs_destroy(void)
{
//    print_debug();
    usleep(1000000);
    struct file* fi = file_list;
    while (fi != NULL) {
        printf("FREEING FILE: %s\n", fi->name);
        free_file(fi);

        fi = fi->next;
    }

    int i = 0;

    while (i < file_descriptor_capacity) {
        struct filedesc* fdi = file_descriptors[i];

        if (fdi != NULL) {
            printf("FREEING FD: %d\n", i);
            free_file_desc(fdi);
        }
        i++;
    }

//    print_debug();

    free(file_descriptors);
}


//#endif