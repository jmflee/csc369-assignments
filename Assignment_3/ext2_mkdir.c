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
      fprintf(stderr, "Usage: mkdir <image file name> <new directory>\n");
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

  char* dirstr = malloc(strlen(argv[2]) + 1);

  strcpy(dirstr, argv[2]);

  int dir_inode = get_item_inode(dirstr, inode_table);
  int parent_inode = get_item_dir(dirstr, inode_table);

  if(dir_inode != -1) {
    printf("Specified directory already exists.\n");
    return -EEXIST;
  }
  else if(parent_inode == -1) {
    printf("Specified path does not exist.\n");
    return -ENOENT;
  }
  else { // valid new directory was given
    strcpy(dirstr, argv[2]);
    char* dirname = malloc(strlen(dirstr));

    set_paths(dirstr, dirname); // gets the new name of the directory to be created

    //
  }

  return 0;
}
