#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "free.h"
#include "image.h"
#include "mkfs.h"
#include "ctest.h"
#include "inode.h"
#include "block.h"
#include "pack.h"
#include "directory.h"
#include "dirbasename.h"

void test_image_open(){   
    char *file = "Hello.txt!";
    CTEST_ASSERT((image_open(file, 0) != -1), "File is not there!");
    remove(file);
}

void test_image_close(){
    char *file = "test.txt";
    image_open(file, 1);
    CTEST_ASSERT(image_close() == 0, "");
    remove(file);
}

void test_bwrite_and_bread(){
	unsigned char test[BLOCK_SIZE] = {0}; 
	image_open("anyfile.txt", 0); 
	bwrite(9, test);
	unsigned char read[BLOCK_SIZE];
	bread(9, read);
	CTEST_ASSERT(memcmp(test, read, 3) == 0, "Testing bwrite and bread");
	image_close();
}

void test_alloc(){
    image_open("anyfile.txt", 0);

    CTEST_ASSERT(alloc() == 0, "empty block map");
    CTEST_ASSERT(alloc() == 1, "non-empty block map");

    image_close();
}

void test_set_free_and_find_free(void) {
    unsigned char data[4096] = {0};
    CTEST_ASSERT(find_free(data) == 0, "Testing empty");

    set_free(data, 0, 1);
    CTEST_ASSERT(find_free(data) == 1, "Testing find_free to 1 and set_free to 1");

    set_free(data, 1, 1);
    CTEST_ASSERT(find_free(data) == 2, "Testing find_free to 2 and set_free to 1");

    set_free(data, 0, 0);
    CTEST_ASSERT(find_free(data) == 0, "Testing find_free to 0 and set_free to 0");
}

void test_ialloc(void) {
	image_open("any.txt", 0);

    struct inode *ialloc_node = ialloc();
    CTEST_ASSERT(ialloc_node->inode_num == 0, "Testing empty inode map");
    CTEST_ASSERT(ialloc_node->size == 0, "Testing size attribute");
    CTEST_ASSERT(ialloc_node->owner_id == 0, "Testing owner_id attribute");   
    CTEST_ASSERT(ialloc_node->flags == 0, "Testing flags attribute");   
    CTEST_ASSERT(ialloc_node->permissions == 0, "Testing permissions attribute");
    CTEST_ASSERT(ialloc_node->ref_count == 1, "Testing ref_count attribute");       
    
    struct inode *ialloc_node_two = ialloc();
    CTEST_ASSERT(ialloc_node_two->inode_num == 1, "Testing non-empty inode map");

    image_close();
    remove("any.txt");
}

void test_find_free_incore_and_find_incore()
{
    image_open("test.txt", 1);
    reset_incore_inodes();

    struct inode *free_node = find_incore_free();
    CTEST_ASSERT(free_node != NULL, "Testing that incore has a free inode");
    CTEST_ASSERT(free_node->ref_count == 0, "Testing that the free inode has a reference count of zero");

    iget(0);
    struct inode *free_node_2 = find_incore_free();
    CTEST_ASSERT(free_node_2 != NULL, "Testing that incore has another free inode");
    CTEST_ASSERT(free_node_2 != free_node, "Testing that the next free inode is different from the previously allocated inode");

    iget(1);
    struct inode *node = find_incore(1);
    CTEST_ASSERT(node != NULL, "Testing that incore has the specific inode");
    CTEST_ASSERT(node->inode_num == 1, "Testing that the specific inode has the correct inode number");
    image_close();
    remove("test.txt");
}

void test_read_and_write_inode(void)
{
	image_open("test.txt", 0);
    unsigned int test_inode_num = 314;
	struct inode *new_inode = find_incore_free();
	new_inode->inode_num = test_inode_num;
	new_inode->size = 456;
	new_inode->owner_id = 7;
	new_inode->permissions = 8;
	new_inode->flags = 9;
	new_inode->link_count = 10;
	new_inode->block_ptr[0] = 11;

	write_inode(new_inode);
	struct inode inode_read_buffer = {0};
	read_inode(&inode_read_buffer, test_inode_num);

	CTEST_ASSERT(new_inode->size == 456, "Testing size attribute");
	CTEST_ASSERT(new_inode->owner_id == 7, "Testing owner_id attribute");
	CTEST_ASSERT(new_inode->permissions == 8, "Testing permissions attribute");
	CTEST_ASSERT(new_inode->flags == 9, "Testing flags attribute");
	CTEST_ASSERT(new_inode->link_count == 10, "Testing link_count attribute");
	CTEST_ASSERT(new_inode->block_ptr[0] == 11, "Testing block pointer attribute");

    image_close();
    remove("test.txt");
}

void test_iget()
{
    image_open("test.txt", 1);
    reset_incore_inodes();

    unsigned int inode_num = 10;
    struct inode *new_node = iget(inode_num);
    CTEST_ASSERT(new_node->inode_num == inode_num, "Testing if iget returns an inode with the specified inode_num.");
    CTEST_ASSERT(new_node->ref_count == 1, "Testing iget for a node that is not in the core updates the ref_count to 1");

    struct inode *node_two = iget(inode_num);
    CTEST_ASSERT(node_two->ref_count == 2, "Testing iget for inode increments the ref count correctly");
    CTEST_ASSERT(node_two == new_node, "Testing iget return the same inode pointer");

    image_close();
    remove("test.txt");
}   

void test_iput()
{
	image_open("test.txt", 0);
	struct inode *test_node = iget(1);
	CTEST_ASSERT(test_node->ref_count == 1, "Testing initial ref_count is 1");

	iput(test_node);
	CTEST_ASSERT(test_node->ref_count == 0, "Testing ref_count is decremented to zero");

	struct inode *iget_inode = iget(1);
	CTEST_ASSERT(iget_inode->ref_count == 1, "Testing that iget increments ref_count correctly");
	CTEST_ASSERT(iget_inode->inode_num == test_node->inode_num, "Testing that iget retrieves the correct inode");
	CTEST_ASSERT(iget_inode->size == test_node->size, "Testing that iget retrieves the correct inode");

	image_close();
	remove("test.txt");
}

void test_mkfs(void) {
    image_open("test_image.txt", 0);
    
    unsigned char outblock[BLOCK_SIZE];
    unsigned char block[BLOCK_SIZE];
    memset(block, 0, BLOCK_SIZE);
    
    mkfs();
    
    CTEST_ASSERT(memcmp(bread(8, outblock), block, BLOCK_SIZE) == 0, "Testing if blocks are correctly set to zero");
    CTEST_ASSERT(alloc() == 8, "Testing if the 'alloc' function returns the expected next free block index");

    struct inode *root_inode;
    unsigned char inode_bitmap_block[BLOCK_SIZE];
    unsigned char initial_block[BLOCK_SIZE];
    
    memset(inode_bitmap_block, 0, BLOCK_SIZE);
    memset(initial_block, 0, BLOCK_SIZE);
    initial_block[0] = 1;

    bread(1, inode_bitmap_block);
    CTEST_ASSERT(memcmp(inode_bitmap_block, initial_block, BLOCK_SIZE) == 0, "Testing the initialization of the inode bitmap");

    int inode_num = read_u16(inode_bitmap_block);
    inode_num = read_u16(inode_bitmap_block + DIR_ENTRY_SIZE);
    root_inode = iget(inode_num);
    CTEST_ASSERT(root_inode->flags == 2, "Testing if root directory is allocated and has the directory flag set");
    CTEST_ASSERT(root_inode->size == DIR_START_SIZE, "Testing if root directory is initialized with the correct size");
    CTEST_ASSERT(root_inode->block_ptr[0] == 7, "Testing if root directory inode points to the 8th data block");

    image_close();
    remove("test_image.txt");
}


void test_directory_open(){
    image_open("test.txt", 0);

    struct inode *new_node = iget(3);
    struct directory *dir = malloc(sizeof(struct directory));

    dir = directory_open(3);
    CTEST_ASSERT(dir->inode == new_node, "Testing if the directory has the correct inode pointer for the given inode number");
    CTEST_ASSERT(dir->offset == 0, "Testing that the initial offset of the returned directory is 0");

    image_close();
    remove("test.txt");
}

void test_directory_get(){
    image_open("test.txt", 0);
    mkfs();

    struct directory *dir;
    struct directory_entry ent;

    dir = directory_open(0);
    directory_get(dir, &ent);
    CTEST_ASSERT(strcmp(ent.name, ".") == 0, "Testing creation of root directory");
    directory_close(dir);
    
    image_close();
    remove("test.txt");
}

void test_directory_close(){
    image_open("test.txt", 0);

    struct directory *dir;

    dir = directory_open(0);
    struct inode *existing_inode = find_incore(0);
    directory_close(dir);
    struct inode *closed_inode = find_incore(0);
    CTEST_ASSERT(existing_inode != NULL && closed_inode == NULL, "Testing that the directory's inode was placed in memory and removed after calling 'directory_close'");

    image_close();
    remove("test.txt");
}

void test_namei()
{
    image_open("test.txt", 0);

    mkfs();

    struct inode *result = namei("/");
    CTEST_ASSERT(result->inode_num == 0, "Testing namei on root directory");

    result = namei("//");
    CTEST_ASSERT(result == NULL, "Testing NULL return for invalid path");

    image_close();
    remove("test.txt");
}

void test_directory_make()
{
    image_open("test.txt", 0);

    struct directory *dir;
    struct directory_entry ent;

    dir = directory_open(0);
    directory_get(dir, &ent);
    directory_get(dir, &ent);
    
    unsigned char inode_block[BLOCK_SIZE] = {0};
    bread(FREE_INODE_BLOCK_NUM, inode_block);

    int free = find_free(inode_block);
    directory_make("/foo");
    directory_get(dir, &ent);
    
    unsigned char data_block[BLOCK_SIZE] = {0};
    struct inode *dir_inode = dir->inode;
    int dir_data_block_num = dir_inode->block_ptr[0];
    bread(dir_data_block_num, data_block);
    int inode_num = read_u16(data_block + 2 * DIR_ENTRY_SIZE);

    CTEST_ASSERT(strcmp(ent.name, "foo") == 0, "Testing directory_make creates directory");
    CTEST_ASSERT(inode_num == free && strcmp(ent.name, "foo") == 0, "Testing directory file storage and inode allocation");
    CTEST_ASSERT(dir_inode->size == DIR_START_SIZE + DIR_ENTRY_SIZE, "Testing parent directory size update");

    image_close();
    remove("test.txt");
}

int main(void)
{
    CTEST_VERBOSE(1);
    test_image_open();
    test_image_close();
    test_alloc();
    test_bwrite_and_bread();
    test_set_free_and_find_free();
    test_ialloc();
    test_find_free_incore_and_find_incore();
    test_read_and_write_inode();
    test_mkfs();
    test_iget();
    test_iput();
    test_directory_open();
    test_directory_get();
    test_directory_close();
    test_namei();
    test_directory_make();
    CTEST_RESULTS();
    CTEST_EXIT();
}
