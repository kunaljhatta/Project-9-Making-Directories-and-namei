#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mkfs.h"
#include "inode.h"
#include "block.h"
#include "free.h"
#include "image.h"
#include "pack.h"
#include "directory.h"
#include "dirbasename.h"

struct directory *directory_open(int inode_num)
{
    struct inode *dir_inode = iget(inode_num);
    if(dir_inode == NULL){
        return NULL;
    }
    struct directory *dir = malloc(sizeof(struct directory));
    dir->inode = dir_inode;
    dir->offset = 0;
    return dir;
}

int directory_get(struct directory *dir, struct directory_entry *ent)
{
    unsigned char block[BLOCK_SIZE];

    struct inode *dir_inode = dir->inode;
    int dir_size = dir_inode->size;
    if((int)dir->offset >= dir_size){
        return -1;
    }

    int data_block_index = dir->offset / BLOCK_SIZE;
    int data_block_num = dir_inode->block_ptr[data_block_index];
    bread(data_block_num, block);
    int offset_in_block = dir->offset % BLOCK_SIZE;
    ent->inode_num = read_u16(block + offset_in_block);
    strcpy(ent->name, (char*)(block + offset_in_block + FILE_NAME_OFFSET));

    dir->offset += DIR_ENTRY_SIZE;
    return 1;
}

void directory_close(struct directory *dir)
{
    iput(dir->inode);
    free(dir);
}

struct inode *namei(char *path)
{
    if(strcmp(path, "/") == 0){
        struct inode *root = iget(ROOT_INODE_NUMBER);
        return root;
    }
    return NULL;
}

int directory_make(char *path)
{
    char parent_directory[MAX_PATH_LENGTH];
    get_dirname(path, parent_directory);

    char directory_name[MAX_FILENAME_LENGTH];
    get_basename(path, directory_name);

    struct inode *parent_inode = namei(parent_directory);
    struct inode *new_directory = ialloc();
    int directory_block_number = alloc();

    if (parent_inode == NULL) {
        return -1;
    }

    unsigned char new_directory_block[BLOCK_SIZE] = {0};
    write_u16(new_directory_block, new_directory->inode_num);
    strcpy((char*)(new_directory_block + FILENAME_OFFSET), ".");
    write_u16(new_directory_block + DIR_ENTRY_SIZE, parent_inode->inode_num);
    strcpy((char*)(new_directory_block + DIR_ENTRY_SIZE + FILENAME_OFFSET), "..");

    new_directory->flags = DIR_FLAG;
    new_directory->size = DIR_START_SIZE;
    new_directory->block_ptr[0] = directory_block_number;
    bwrite(directory_block_number, new_directory_block);

    int block_ptr_index = parent_inode->size / BLOCK_SIZE;
    int block_number_in_parent = parent_inode->block_ptr[block_ptr_index];

    unsigned char parent_directory_block[BLOCK_SIZE] = {0};
    bread(block_number_in_parent, parent_directory_block);

    int offset_in_block = parent_inode->size % BLOCK_SIZE;
    write_u16(parent_directory_block + offset_in_block, new_directory->inode_num);
    strcpy((char*)(parent_directory_block + offset_in_block + FILENAME_OFFSET), directory_name);

    bwrite(block_number_in_parent, parent_directory_block);
    parent_inode->size += DIR_ENTRY_SIZE;

    iput(new_directory);
    iput(parent_inode);

    return 0;
}
