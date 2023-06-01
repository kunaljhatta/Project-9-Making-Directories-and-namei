#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mkfs.h"
#include "block.h"
#include "image.h"
#include "inode.h"
#include "pack.h"
#include "ls.h"
#include "directory.h"


void mkfs(void)
{
    unsigned char zero_block[BLOCK_SIZE * NUMBER_OF_BLOCKS] = { 0 };

    write(image_fd, zero_block, BLOCK_SIZE * NUMBER_OF_BLOCKS);
    for(int i=0; i < 7; i++){
        alloc();
    }
    struct inode *root_inode = ialloc();
    int root_block_num = alloc();
    root_inode->flags = DIR_FLAG;
    root_inode->size = DIR_START_SIZE;
    root_inode->block_ptr[0] = root_block_num;

    unsigned char block[BLOCK_SIZE] = { 0 };
    write_u16(block, root_inode->inode_num);
    strcpy((char*)(block + FILE_NAME_OFFSET), ".");
    write_u16(block + DIR_ENTRY_SIZE, root_inode->inode_num);
    strcpy((char*)(block + DIR_ENTRY_SIZE + FILE_NAME_OFFSET), "..");
    bwrite(root_block_num, block);
    iput(root_inode);
}
