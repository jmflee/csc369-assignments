#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_utils.h"

unsigned char *disk;

int main(int argc, char **argv) {
    //int i;

    if(argc != 4) {
        fprintf(stderr, "Usage: cp <image file name> <source path> <destination path>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    // Get required block size of the source file
    int src_fd = open(argv[2], O_RDONLY);
    int src_size = lseek(src_fd, 0L, SEEK_END);
    int block_size = (src_size - 1) / EXT2_BLOCK_SIZE + 1;
    unsigned char *file_size = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(file_size == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *super_block = (void *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *bgd = (void *)(disk + (EXT2_BLOCK_SIZE *  2));
	struct ext2_inode *inode_table = (void *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_table));
    unsigned int *inode_bitmap = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_bitmap));
	unsigned int *block_bitmap = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_block_bitmap));
    
    // No more free space in disk
    if (super_block->s_free_blocks_count < block_size) {
        fprintf(stderr, "cp: cannot copy '%s\n', no free blocks left in disk", argv[2]);
        return -(ENOSPC);
    }
    // No more inodes left in the disk
    if (super_block->s_free_inodes_count == 0) {
        fprintf(stderr, "cp: cannot copy '%s\n', no free inodes left in disk", argv[2]);
        return -(ENOSPC);
    }
    
    char srcfile[256];
    char *srcpath = malloc(sizeof(char *));
    strncpy(srcpath, argv[2], strlen(argv[2]));
    int srcdir_inode = get_dir_inode(srcpath, inode_table); // Get inode # of directory
    // User entered a directory with no file
    if (srcdir_inode != -1) {
        fprintf(stderr, "cp: omitting directory '%s'\n", argv[2]);
        return -(EISDIR); 
    } else {
        strncpy(srcpath, argv[2], strlen(argv[2]));
        set_paths(srcpath, srcfile); // Get path and file names 
        srcdir_inode = get_dir_inode(srcpath, inode_table); // Get inode # of directory
    }
    if (srcdir_inode == -1) { // source directory does not exists
        fprintf(stderr, "cp: cannot copy '%s': No such file or directory\n", argv[2]);
        return -(ENOENT); 
    }
    // File exists so we get the inode of the specific file
    struct ext2_inode *src_inode = inode_table + srcdir_inode;
    unsigned int file_inode = get_file_inode(src_inode, srcfile);
    
    if (file_inode == -1) { // source file does not exists
        fprintf(stderr, "cp: cannot copy '%s': No such file or directory\n", argv[2]);
        return -(ENOENT); 
    }
    
    char destfile[256];
    char *destpath = malloc(sizeof(char *));
    strncpy(destpath, argv[3], strlen(argv[3]));
    int destdir_inode = get_dir_inode(destpath, inode_table);
    // User specified a specific name for the copied file to be named
    if (destdir_inode == -1) {
        strncpy(destpath, argv[3], strlen(argv[3]));
        set_paths(destpath, destfile); // Get path and file names 
        destdir_inode = get_dir_inode(destpath, inode_table); // Get inode # of directory 
    } else {
        strncpy(destfile, srcfile, strlen(srcfile));
    }
    struct ext2_inode *dest_inode = inode_table + destdir_inode;
    unsigned int path_inode = get_file_inode(dest_inode, destfile);
    if (path_inode != -1) { 
        fprintf(stderr, "cp: cannot copy to '%s': File already exists\n", argv[3]);
        return -(EEXIST); 
    }
    
    // Set up the new inode
    unsigned int freshnode = get_free_inode();
    add_bitmap (inode_bitmap,freshnode);
    // Adding in the new inode
    struct ext2_inode *fresh_inode = &(inode_table[freshnode - 1]);
    super_block->s_free_inodes_count--; // decrement free inode counter

    // Setup the new blocks for the new inode
    int current;
    for (current = 0; current < block_size && current < 13; current++) {
        int freshblock = get_free_block();
        add_bitmap (block_bitmap, freshblock);
    }
    // Populating a blank inode
    fresh_inode->i_mode = EXT2_S_IFREG; 
    fresh_inode->i_size = src_size; 
    fresh_inode->i_links_count = 1; 
    fresh_inode->i_blocks = block_size * 2;
    // Populating the blocks
    for (current = 0; current < block_size && current < 15; current++) {
        //Only clone crucial blocks
        if(current < 12 && current < block_size) {
            fresh_inode->i_block[current] = get_free_block();
            add_bitmap(block_bitmap, fresh_inode->i_block[current]);
            // Rewriting blocks with src inode
            memcpy((disk + (EXT2_BLOCK_SIZE * fresh_inode->i_block[current])), file_size, block_size); 
            super_block->s_free_blocks_count--;
        } else { 
            fresh_inode->i_block[current] = 0;
        }
    }
    int x = 0;;
    // We need more blocks so we add indirect blocks and copy the file block by block again
    if (current >= 12) {
        unsigned int *moar_blocks = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * fresh_inode->i_block[12])); 
        while (current < block_size) {
            moar_blocks[x] = get_free_block();
            add_bitmap(block_bitmap, moar_blocks[x]);
            // Copying indirect blocks
            memcpy((disk + (EXT2_BLOCK_SIZE * (moar_blocks[x] + 1))), file_size + (EXT2_BLOCK_SIZE * x), EXT2_BLOCK_SIZE); 
            super_block->s_free_blocks_count--;
            x++;
        }
    }
    struct ext2_dir_entry_2 *fresh_de;
    // Look for an inode with enough space to occupy in the destination
    for (x = 0; x < 12 && dest_inode->i_block[x]; x++) {
        current = 0; 
        struct ext2_dir_entry_2 *de = (void *)(disk + (EXT2_BLOCK_SIZE * dest_inode->i_block[x]));
        while (current < EXT2_BLOCK_SIZE) { // Iterating through the blocks and finding where rec_len ends
            if (de->rec_len >= (2 * sizeof(struct ext2_dir_entry_2)) + strlen(destfile) + (4 - (strlen(destfile) % 4)) + de->name_len + (4 - (de->name_len % 4 ))) {
                // Populating dir entry
                fresh_de = (struct ext2_dir_entry_2 *)((char *)de + (sizeof(struct ext2_dir_entry) + de->name_len + (de->name_len % 4)));
                fresh_de->rec_len = de->rec_len - (sizeof(struct ext2_dir_entry) + de->name_len + (de->name_len % 4));
                fresh_de->inode = freshnode;
                fresh_de->name_len = strlen(destfile);
                fresh_de->file_type = EXT2_FT_DIR;
                strncpy(fresh_de->name, destfile, fresh_de->name_len);

                de->rec_len = (sizeof(struct ext2_dir_entry) + de->name_len + (de->name_len % 4));
                return 0;
            }
            if (!de->rec_len) break;
            de += de->rec_len;
            current += de->rec_len;
        }
    }
    // There was no free block
    if (!dest_inode->i_block[x]) {
        dest_inode->i_block[x] = get_free_block();
        add_bitmap(block_bitmap, dest_inode->i_block[x]);
        fresh_de = (struct ext2_dir_entry_2 *) (disk + (EXT2_BLOCK_SIZE * dest_inode->i_block[x]));
        fresh_de->inode = freshnode;
        fresh_de->rec_len = EXT2_BLOCK_SIZE;
        fresh_de->name_len = strlen(destfile);
        fresh_de->file_type = EXT2_FT_DIR;
        strncpy(fresh_de->name, destfile, fresh_de->name_len);
        
        dest_inode->i_size += EXT2_BLOCK_SIZE;
        dest_inode->i_blocks += 2;
    }
    free(srcpath);
    free(destpath);
    return 0;
}
