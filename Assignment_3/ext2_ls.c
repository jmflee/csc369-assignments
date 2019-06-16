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

  if(argc != 3) {
      fprintf(stderr, "Usage: ls <image file name> <path>\n");
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

  int dir_inode =  get_dir_inode(argv[2], inode_table);

  if(dir_inode == -1) { // inode for the given directory not found
    fprintf(stderr, "ls: '%s' : Not a directory\n", argv[2]);
    return -(ENOENT);
  }
  else { // valid directory was given
    print_inode_dirs(inode_table, dir_inode);
  }

  return 0;
}
