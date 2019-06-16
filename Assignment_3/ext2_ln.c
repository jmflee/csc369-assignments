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
  if(argc != 4) {
      fprintf(stderr, "Usage: ln <image file name> <from> <to>\n");
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

  char* fromstr = malloc(strlen(argv[2]) + 1);
  char* tostr = malloc(strlen(argv[3]) + 1);

  strcpy(fromstr, argv[2]);
  strcpy(tostr, argv[3]);

  int from =  get_item_inode(fromstr, inode_table);
  int to =  get_item_inode(tostr, inode_table);

  strcpy(fromstr, argv[2]);
  strcpy(tostr, argv[3]);

  if(from == -1) {
    printf("From location does not exist.\n");
    return -ENOENT;
  }
  else if(to != -1) {
    printf("To location already exists.\n");
    return -EEXIST;
  }
  else if(get_dir_inode(fromstr, inode_table) != -1 || get_dir_inode(tostr, inode_table) != -1) {
    printf("Can not ln directories.\n");
    return -EISDIR;
  }
  else { // valid directory was given
    strcpy(tostr, argv[3]);
    int itemdir = get_item_dir(tostr, inode_table); // get the dir of the link
    strcpy(tostr, argv[3]);
    char* itemname = malloc(strlen(tostr));
    set_paths(tostr, itemname); // get the name of the link we want to make

    struct ext2_inode *frominode = inode_table + from; // inode of the file we are linking
    struct ext2_inode *in = inode_table + itemdir; // directory of where the link will be in
    struct ext2_dir_entry_2 *to_de = (void *)disk + EXT2_BLOCK_SIZE * in->i_block[0]; // de of parent dir of the link
    struct ext2_dir_entry_2 *last_de = to_de; // stores last processed de
    int i = 0;
    int total_len = 0;

    while(i < EXT2_BLOCK_SIZE) { // get last directory entry
      total_len += to_de->rec_len;
      if(!to_de->rec_len) break;
     	i+= to_de->rec_len;
      last_de = to_de;
     	to_de = (void *)to_de + to_de->rec_len;
    }

    total_len -= last_de->rec_len; // get length left
    last_de->rec_len = 4*last_de->name_len + 8; // calculate rec_len needed for this entry
    total_len += last_de->rec_len;
    last_de = (void *)last_de + last_de->rec_len; //enter the new dir entry
    last_de->inode = from + 1; // populate the fields
    last_de->rec_len = EXT2_BLOCK_SIZE - total_len;
    last_de->name_len = strlen(itemname);
    strcpy(last_de->name, itemname);
    frominode->i_links_count ++; // increase link count
  }

  return 0;
}
