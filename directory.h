#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_ENTRY_SIZE 32
#define DIR_START_SIZE DIR_ENTRY_SIZE * 2
#define FILE_NAME_OFFSET 2
#define FILENAME_OFFSET 2

#define MAX_FILENAME_LENGTH 16
#define MAX_PATH_LENGTH 2175
#define ROOT_INODE_NUMBER 0

struct directory {
    struct inode *inode;
    unsigned int offset;
};

struct directory_entry {
    unsigned int inode_num;
    char name[16];
};

struct directory *directory_open(int inode_num);
int directory_get(struct directory *dir, struct directory_entry *ent);
void directory_close(struct directory *d);
struct inode *namei(char *path);
int directory_make(char *path);

#endif