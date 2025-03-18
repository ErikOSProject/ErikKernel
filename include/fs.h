/**
 * @file fs.h
 * @brief Filesystem functions.
 *
 * This file contains the declarations for the filesystem functions.
 */

#ifndef _FS_H
#define _FS_H

#include <stddef.h>
#include <stdint.h>

typedef struct fs_node fs_node;
typedef struct fs_mount_point fs_mount_point;

extern fs_mount_point *fs_mounts;

typedef struct {
	int (*init)(void *data);
	int (*find_node)(void *data, fs_node *node, const char *path);
	int (*read)(void *data, char *out, size_t cursor, size_t n);
	void *(*mkdir)(void *data, const char *path);
	void *(*mkfile)(void *data, const char *path);
} fs_driver;

typedef enum {
	INVALID,
	FILE,
	DIRECTORY,
	SYMLINK,
} fs_node_type;

struct fs_mount_point {
	char *path;
	fs_driver *driver;
	void *data;
	fs_mount_point *next;
};

struct fs_node {
	fs_node_type type;
	fs_driver *driver;
	void *data;
	fs_mount_point *mount;
	size_t cursor;
	size_t size;
};

typedef enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
} fs_seek_mode;

static inline int fs_read(fs_node *node, char *out, size_t n)
{
	return node->driver->read(node->data, out, node->cursor, n);
}

static inline size_t fs_seek(fs_node *node, size_t offset, fs_seek_mode mode)
{
	switch (mode) {
	case SEEK_SET:
		return node->cursor = offset;
	case SEEK_CUR:
		return node->cursor += offset;
	case SEEK_END:
		return node->cursor = node->size + offset;
	}
}

fs_mount_point *fs_mount_point_for_path(const char *path, size_t *index);
int fs_find_node(fs_node *node, const char *path);
void fs_init(BootInfo *boot_info);

#endif //_FS_H
