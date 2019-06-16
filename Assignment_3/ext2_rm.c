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

    if(argc != 3) {
        fprintf(stderr, "Usage: rm <image file name> <path>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));
	struct ext2_inode *inode_table = (struct ext2_inode *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_table));

    char filename[256];
    char *pathname = malloc(sizeof(char *));
    strncpy(pathname, argv[2], strlen(argv[2]));
    set_paths(pathname, filename); // Get path and file names
    int dir_inode = get_dir_inode(pathname, inode_table); // Get inode # of directory
    if(dir_inode == -1) { // inode for the given directory not found or when not using absolute paths
        fprintf(stderr, "rm: cannot remove '%s': No such file or directory\n", argv[2]);
        return -(ENOENT);
    }
    else { // valid directory was given
        switch(del_file(inode_table, dir_inode, filename)) {
            case -1: // Trying to remove a non-existent file
            fprintf(stderr, "rm: cannot remove '%s': No such file or directory\n", argv[2]);
            return -(ENOENT);
            case -2: // Trying to remove a directory
            fprintf(stderr, "ext2_rm: cannot remove '%s': Is a directory\n", argv[2]);
            return -(EISDIR);

        }
    }
    free(pathname);
    return 0;
}
