/**
 * @file fs.c
 * @brief Filesystem implementation for the ErikKernel.
 *
 * This file contains the implementation of the virtual filesystem used within the ErikKernel.
 * It also provides ramfs (RAM-based filesystem) support and loading of the initial ramdisk to a ramfs root.
 */

#include <debug.h>
#include <erikboot.h>
#include <fs.h>
#include <heap.h>
#include <memory.h>

static int ramfs_find_node(void *data, fs_node *node, const char *path);
static int ramfs_read(void *data, char *out, size_t cursor, size_t n);
static void *ramfs_mkdir(void *data, const char *path);
static void *ramfs_mkfile(void *data, const char *path);

typedef struct ramfs_node ramfs_node;
struct ramfs_node {
	char *path;
	fs_node_type type;
	ramfs_node *parent;
	ramfs_node *next;
	ramfs_node *children;
};

typedef struct ramfs_file ramfs_file;
struct ramfs_file {
	ramfs_node node;
	char *data;
	size_t length;
};

fs_mount_point *fs_mounts = NULL;
fs_driver ramfs_driver = {
	NULL, ramfs_find_node, ramfs_read, ramfs_mkdir, ramfs_mkfile,
};

/**
 * @brief Retrieves the mount point for a given path.
 *
 * This function searches for the mount point associated with the specified path.
 * If a mount point is found, it returns a pointer to the mount point structure.
 * Additionally, it can provide the index of the mount point in the internal list.
 *
 * @param path The path for which to find the mount point.
 * @param index A pointer to a size_t variable where the index of the mount point will be stored.
 *              If the index is not needed, this parameter can be NULL.
 * @return A pointer to the fs_mount_point structure corresponding to the path, or NULL if no mount point is found.
 */
fs_mount_point *fs_mount_point_for_path(const char *path, size_t *index)
{
	fs_mount_point *mount = NULL;
	size_t match = 0;

	for (fs_mount_point *m = fs_mounts; m; m = m->next) {
		bool possible = true;
		size_t i = 0;
		for (; m->path[i]; ++i) {
			if (m->path[i] != path[i]) {
				possible = false;
				break;
			}
		}

		if (possible && i > match) {
			match = i;
			mount = m;
		}
	}

	if (index)
		*index = match;

	return mount;
}

/**
 * @brief Finds a file system node based on the given path.
 *
 * This function searches for a file system node within the file system using the provided path.
 *
 * @param node The fs_node structure that will be updated with the found node's details.
 * @param path The path to the desired file system node.
 * @return An integer indicating the result of the search.
 *         Zero indicates success, while a non-zero value indicates failure.
 */
int fs_find_node(fs_node *node, const char *path)
{
	size_t mp_index = 0;
	fs_mount_point *mount = fs_mount_point_for_path(path, &mp_index);
	if (!mount || !mount->driver->find_node)
		return -1;
	node->mount = mount;
	return mount->driver->find_node(mount->data, node, path + mp_index);
}

/**
 * @brief Converts an octal string to its binary representation.
 *
 * This function takes a string representing an octal number and converts it
 * to its binary equivalent. The conversion is performed up to the specified
 * size.
 *
 * This function is used to parse the octal fields in the tar archive headers.
 *
 * @param str The string containing the octal number.
 * @param size The number of characters in the string to convert.
 * @return The binary representation of the octal number.
 */
static int oct2bin(char *str, int size)
{
	int n = 0;
	char *c = str;
	while (size-- > 0) {
		n *= 8;
		n += *c - '0';
		c++;
	}
	return n;
}

/**
 * @brief Loads the initial ramdisk (initrd) into memory.
 *
 * This function is responsible for loading the initial ramdisk (initrd) 
 * using the provided boot information. The initrd is typically used 
 * during the early stages of the boot process to provide a temporary 
 * root filesystem before the real root filesystem is mounted.
 *
 * @param boot_info A pointer to the BootInfo structure containing 
 *                  information about the boot process.
 */
static void load_initrd(BootInfo *boot_info)
{
	char *i_ptr = boot_info->InitrdBase;
	while ((size_t)(i_ptr - boot_info->InitrdBase) <
	       boot_info->InitrdSize) {
		if (memcmp(i_ptr + 257, "ustar", 5) != 0)
			break;
		size_t filesize = oct2bin(i_ptr + 0x7c, 11);
		if (i_ptr[156] == '0') {
			char *tok_path = malloc(strlen(i_ptr) + 1);
			strcpy(tok_path, i_ptr);

			char *tok = strtok(tok_path, "/");
			char *ptok;
			ramfs_node *parent = fs_mounts->data;

			while (tok) {
				ptok = tok;
				tok = strtok(NULL, "/");
				if (!tok)
					break;

				bool exists = false;
				for (ramfs_node *c = parent->children; c;
				     c = c->next)
					if (strcmp(ptok, c->path) == 0) {
						parent = c;
						exists = true;
					}
				if (exists)
					continue;

				parent = ramfs_mkdir(parent, ptok);
			}

			ramfs_file *file = ramfs_mkfile(parent, ptok);
			file->length = filesize;
			file->data = i_ptr + 512;
			free(tok_path);
		}
		i_ptr += (((filesize + 511) / 512) + 1) * 512;
	}
}

/**
 * @brief Initializes the virtual filesystem.
 *
 * This function sets up the virtual filesystem using the provided boot information.
 *
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 */
void fs_init(BootInfo *boot_info)
{
	ramfs_node *tmproot = malloc(sizeof(ramfs_node));
	tmproot->path = malloc(1);
	strcpy(tmproot->path, "");
	tmproot->type = DIRECTORY;
	tmproot->parent = NULL;
	tmproot->next = NULL;
	tmproot->children = NULL;

	fs_mounts = malloc(sizeof(fs_mount_point));
	fs_mounts->next = NULL;
	fs_mounts->path = malloc(2);
	strcpy(fs_mounts->path, "/");
	fs_mounts->driver = &ramfs_driver;
	fs_mounts->data = (void *)tmproot;

	if (ramfs_driver.init)
		ramfs_driver.init((void *)tmproot);

	if (boot_info->InitrdBase) {
		load_initrd(boot_info);
	}
}

/**
 * @brief Finds a node in the RAM filesystem based on the given path.
 *
 * This function searches for a filesystem node within the RAM filesystem
 * using the provided path. It updates the provided node structure with
 * the details of the found node if the search is successful.
 *
 * @param data A pointer to the filesystem-specific data.
 * @param node A pointer to the fs_node structure that will be updated with the found node's details.
 * @param path A string representing the path to search for within the RAM filesystem.
 * @return An integer indicating the success or failure of the search operation.
 *         Zero indicates success, while a non-zero value indicates failure.
 */
static int ramfs_find_node(void *data, fs_node *node, const char *path)
{
	if (!data || !node || !path)
		return -1;

	ramfs_node *fs = (ramfs_node *)data;
	ramfs_node *ramnode = fs;

	char *tok_path = malloc(strlen(path) + 1);
	strcpy(tok_path, path);
	char *tok = strtok(tok_path, "/");

	while (tok) {
		ramfs_node *new_ramnode = NULL;
		for (ramfs_node *n = ramnode->children; n; n = n->next) {
			if (strcmp(tok, n->path) == 0) {
				new_ramnode = n;
				break;
			}
		}

		if (new_ramnode)
			ramnode = new_ramnode;
		else {
			free(tok_path);
			return -1;
		}

		tok = strtok(NULL, "/");
	}

	free(tok_path);

	node->type = ramnode->type;
	node->driver = &ramfs_driver;
	node->data = (void *)ramnode;
	node->cursor = 0;

	if (ramnode->type == FILE) {
		ramfs_file *ramfile = (ramfs_file *)ramnode;
		node->size = ramfile->length;
	}

	return 0;
}

/**
 * @brief Reads data from a RAM-based filesystem.
 *
 * This function reads data from a file in the RAM-based filesystem.
 * It copies the data from the specified file to the provided buffer
 * starting from the specified cursor position and up to the specified
 * number of bytes.
 *
 * @param data Pointer to the filesystem-specific node structure.
 * @param out Pointer to the buffer where the read data will be stored.
 * @param cursor The position in the file from where to start reading.
 * @param n The maximum number of bytes to read.
 * @return The number of bytes actually read, or a negative error code on failure.
 */
static int ramfs_read(void *data, char *out, size_t cursor, size_t n)
{
	if (!data || !out)
		return -1;
	ramfs_node *ramnode = data;

	if (ramnode->type != FILE)
		return -1;

	ramfs_file *file = (ramfs_file *)ramnode;
	if (cursor + n > file->length)
		return -1;

	memcpy(out, file->data + cursor, n);
	return 0;
}

/**
 * @brief Creates a directory in the RAM filesystem.
 *
 * This function creates a new directory at the specified path within the RAM filesystem.
 *
 * @param data A pointer to the parent directory where the new directory should be created.
 * @param path The path where the new directory should be created.
 * @return A pointer to the created directory, or NULL if the operation fails.
 */
static void *ramfs_mkdir(void *data, const char *path)
{
	ramfs_node *dir = malloc(sizeof(ramfs_node));
	dir->path = malloc(strlen(path) + 1);
	strcpy(dir->path, path);
	dir->type = DIRECTORY;
	dir->parent = data;
	dir->next = NULL;
	dir->children = NULL;
	if (dir->parent->children) {
		ramfs_node *prev = dir->parent->children;
		while (prev->next)
			prev = prev->next;
		prev->next = dir;
	} else
		dir->parent->children = dir;
	return dir;
}

/**
 * @brief Creates a new file in the RAM filesystem.
 *
 * This function creates a new file at the specified path within the RAM filesystem.
 *
 * @param data A pointer to the parent directory where the new file should be created.
 * @param path The path where the new file should be created.
 * @return Pointer to the newly created file, or NULL on failure.
 */
static void *ramfs_mkfile(void *data, const char *path)
{
	ramfs_file *f = malloc(sizeof(ramfs_file));
	f->node.path = malloc(strlen(path) + 1);
	strcpy(f->node.path, path);
	f->node.type = FILE;
	f->node.parent = data;
	f->node.next = NULL;
	f->node.children = NULL;
	if (f->node.parent->children) {
		ramfs_node *prev = f->node.parent->children;
		while (prev->next)
			prev = prev->next;
		prev->next = &f->node;
	} else
		f->node.parent->children = &f->node;
	return f;
}
