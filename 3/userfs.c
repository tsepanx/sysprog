
//#ifndef AAA
//#define AAA

#include "userfs.h"
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "my.c"

enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	/** Block memory. */
	char *memory;
	/** How many bytes are occupied. */
	int occupied;
	/** Next block in the file. */
	struct block *next;
	/** Previous block in the file. */
	struct block *prev;

	/* PUT HERE OTHER MEMBERS */
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
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
	struct file *file;

    int ptr_block_i;
    int ptr_block_offset;

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

struct block* init_block(struct block* prev, struct block* next) {
    struct block* b = malloc(sizeof(struct block));
    b->memory = malloc(BLOCK_SIZE * sizeof(char));
    b->occupied = 0;
    b->next = next;
    b->prev = prev;

    return b;
}

struct file* init_file(const char* filename, struct file* prev, struct file* next) {
    struct file* f = malloc(sizeof(struct file));
    f->name = (char *) filename;

    struct block* b = init_block(NULL, NULL);
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

    return fd;
}

void free_block(struct block* b) {
    free(b->memory);
    free(b);
}

void free_file(struct file* f) {
    struct block* bi = f->block_list;
    while (bi <= f->last_block) {
        free_block(bi);
        bi++;
    }
    free(f);
}

void free_file_desc(struct filedesc* fd) {
    assert(fd != NULL);
//    assert(*fd != NULL);
    free(fd);
}

void add_block(struct file* f) {
    struct block* b = init_block(f->last_block, NULL);

    f->last_block->next = b;
    f->last_block = b;
}

void add_file_to_list(struct file* f, struct file** f_list) {
    struct file* f_i = *f_list;

    if (*f_list == NULL) {
        *f_list = f;
        return;
    }

    while (1) {
        if (f_i->next != NULL) {
            f_i = f_i->next;
        } else {
            break;
        }
    }

    f_i->next = f;
    f->prev = f_i;
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

        file_descriptors = realloc(file_descriptors, file_descriptor_capacity * sizeof(struct filedesc*));
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

void destroy_file(struct file* f) {
    remove_file_from_list(f, &file_list);
    free_file(f);
}

int destroy_fd(int i) {
    if (i < 0 || i >= file_descriptor_capacity || file_descriptors[i] == NULL) {
        return -1;
    }

    free_file_desc(file_descriptors[i]);
    remove_fd_from_list_by_i(i);

    return 0;
}

struct file* find_existing_filename(struct file *f_list, const char* filename) {
    struct file* fi = f_list;

    while (fi != NULL) {
        if (fi->name == filename) {
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

    printf("OPEN: %s, %d\n", filename, flags);

    //    static struct file *file_list = NULL;
    struct file *existing_file = find_existing_filename(file_list, filename);
    struct file *res_file;
    struct filedesc *res_fd;

    if (flags == UFS_CREATE) {
        if (existing_file == NULL) {
            res_file = init_file(filename, NULL, NULL);
            add_file_to_list(res_file, &file_list);
        }
    } else if (flags == 0) {                                        // OPEN EXISTING
        if (existing_file == NULL) {
            ufs_error_code = UFS_ERR_NO_FILE;
            return -1;
        }

        res_file = existing_file;
        res_file->refs++;
    }

    res_fd = init_fd(res_file);
    int fd_i = add_fd_to_list(res_fd);
    return fd_i;
}

ssize_t
ufs_write(int fd, const char *buf, size_t size)
{

    printf("WRITE: %d, %s, %lu\n", fd, buf, size);

	/* IMPLEMENT THIS FUNCTION */
	(void)fd;
	(void)buf;
	(void)size;
	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
    printf("READ: %d, %s, %lu\n", fd, buf, size);

	/* IMPLEMENT THIS FUNCTION */
	(void)fd;
	(void)buf;
	(void)size;
	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

int
ufs_close(int fd)
{
    printf("CLOSE: %d\n", fd);
	/* IMPLEMENT THIS FUNCTION */
//	(void)fd;

//	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
//	return -1;

    return destroy_fd(fd);
}

int
ufs_delete(const char *filename)
{
    printf("DELETE: %s\n", filename);
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
}


//#endif