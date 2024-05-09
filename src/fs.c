
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

int fs_find_node(fs_node *node, const char *path)
{
	size_t mp_index = 0;
	fs_mount_point *mount = fs_mount_point_for_path(path, &mp_index);
	if (!mount || !mount->driver->find_node)
		return -1;
	node->mount = mount;
	return mount->driver->find_node(mount->data, node, path + mp_index);
}

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
