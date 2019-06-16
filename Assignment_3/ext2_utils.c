#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "ext2.h"
#include "ext2_utils.h"

extern unsigned char * disk;

//  Prints the bitmap of data/inode
char* get_bit_map(char *bitmap, int count){
    int i, c;
    char *map = malloc(count + 1);

    for (i = 0; i < count / 8; i++) {
        for (c = 0; c < 8; c++) {
            map[(i * 8) + c] = (bitmap[i] & 1 << c)?'1':'0';
            printf("%c", map[(i * 8) + c]);
        }
        printf(" ");
    }
    map[count] = '\0';
    printf("\n");

    return map;
}

// Prints out the inode
void print_inode(struct ext2_inode *inode, int i){
    int c;
    char type;
    struct ext2_inode *in = inode + i;

    switch(in->i_mode >> 12){
        case(0x4): type = 'd'; break;
        case(0x8): type = 'f'; break;
        default:   type = '?'; break;
    }

    printf("[%d] type: %c size: %d links: %d blocks: %d\n",
        i+1, type, in->i_size, in->i_links_count, in->i_blocks);
    printf("[%d] Blocks:", i+1);

    for (c = 0; c < 12; c++) if (in->i_block[c]) printf(" %d", in->i_block[c]);
    printf("\n");
}

// Sends a a specific inode to print
void print_inode_dirs(struct ext2_inode *inode, int i) {
    int y = 0;
    char name[EXT2_BLOCK_SIZE];
    struct ext2_inode *in = inode + i;
    struct ext2_dir_entry_2 *de;
    if(in->i_mode >> 12 != 0x4) return;
    int x;
    for (x = 0; x < 12 && inode->i_block; x++) {
        de = (void *) (disk + (EXT2_BLOCK_SIZE * in->i_block[x]));
        while(y < EXT2_BLOCK_SIZE) {
            memcpy(name, de->name, de->name_len);
            name[de->name_len] = 0;
            printf("%s\n", name);
            if(!de->rec_len) break;
            y+= de->rec_len;
            de = (void *)de + de->rec_len;
        }
    }
    // Checking for indirect blocks
    if (x == 12) {
        unsigned int *indirect_location = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * inode->i_block[12]));
        for (x = 0; indirect_location[x]; x++) {
            de = (void *) (disk + (EXT2_BLOCK_SIZE * indirect_location[x]));
            while(y < EXT2_BLOCK_SIZE) {
                memcpy(name, de->name, de->name_len);
                name[de->name_len] = 0;
                printf("%s\n", name);
                if(!de->rec_len) break;
                y+= de->rec_len;
                de = (void *)de + de->rec_len;
            }
        }
    }
}

// Returns an int of where the inode exists in the spcific directory
int get_dir_inode(char* dirstr, struct ext2_inode *inode) {
  if(strcmp(dirstr, "/") == 0 || strlen(dirstr) == 0) {
    return 1;
  }

  int ret_inode = -1;
  int end = 0;
  struct ext2_inode *in = inode + 1;
  struct ext2_dir_entry_2 *de = (void *)disk + EXT2_BLOCK_SIZE * in->i_block[0];
  char* namepart = strtok(dirstr, "/");

  while(end == 0 && namepart != NULL) {
    int i = 0;
    int found = 0;
    char name[EXT2_BLOCK_SIZE];

    while(i < EXT2_BLOCK_SIZE && found == 0) {
    	memcpy(name, de->name, de->name_len);
    	name[de->name_len] = 0;

      if(strcmp(name, namepart) == 0) {
        found = 1;
        ret_inode = de->inode - 1;
        in = inode + ret_inode;
      	if(in->i_mode >> 12 != 0x4) return -1;
        de = (void *)disk + EXT2_BLOCK_SIZE * in->i_block[0];
      }
      else {
        if(!de->rec_len) break;
       	i+= de->rec_len;
       	de = (void *)de + de->rec_len;
      }
    }

    if(found == 0) { // path invalid, invalid directory given
      end = 1;
      ret_inode = -1;
    }

    namepart = strtok(NULL, "/");
  }

  return ret_inode;
}

int get_item_dir(char* dirstr, struct ext2_inode *inode) { // strips out the "file" portion of a path
  int i = 0;

  for(i = strlen(dirstr) - 1; i >= 0; i--) { // sets last / to null term
    if(dirstr[i] == '/') {
      dirstr[i] = '\0';
      i = -1;
    }
  }

  if(strlen(dirstr) == 0) {
    return 1;
  }
  else {
    return get_dir_inode(dirstr, inode);
  }
}

int get_item_inode(char* dirstr, struct ext2_inode *inode) { // returns the inode of an item given the inode table and path
  if(strcmp(dirstr, "/") == 0 || strlen(dirstr) == 0) {
    return 1;
  }

  int ret_inode = -1;
  int end = 0;
  struct ext2_inode *in = inode + 1;
  struct ext2_dir_entry_2 *de = (void *)disk + EXT2_BLOCK_SIZE * in->i_block[0];
  char* namepart = strtok(dirstr, "/");

  while(end == 0 && namepart != NULL) {
    int i = 0;
    int found = 0;
    char name[EXT2_BLOCK_SIZE];

    while(i < EXT2_BLOCK_SIZE && found == 0) {
    	memcpy(name, de->name, de->name_len);
    	name[de->name_len] = 0;

      if(strcmp(name, namepart) == 0) {
        found = 1;
        ret_inode = de->inode - 1;
        in = inode + ret_inode;
        de = (void *)disk + EXT2_BLOCK_SIZE * in->i_block[0];
      }
      else {
        if(!de->rec_len) break;
       	i+= de->rec_len;
       	de = (void *)de + de->rec_len;
      }
    }

    if(found == 0) { // path invalid, invalid directory given
      end = 1;
      ret_inode = -1;
    }

    namepart = strtok(NULL, "/");
  }

  return ret_inode;
}

/* Sets the pointer of a fullpath and splits into a path and a file
 * ex: before path = doo/foo/poo file = NULL
 *     after  path = doo/foo/     file = poo
 */
void set_paths (char *path, char *file) {
    int i = strlen(path)-1;
    // Find a /
    while (path[i] != '/' && i >= 0) {
        i--;
    }
    // Increase so / doesn't show in file
    if (path[i] == '/') {
        i++;
    }
    // Copy the file name
    strncpy(file, path + i, strlen(path) - i);
    // Get just directory names wihtout files
    path[i] = 0;
    file[strlen(file)] = '\0'; // Add null terminator to file
    path[strlen(path)] = '\0'; // Add null termination to path
}

// Sends a a specific inode to delete, returns -1 or -2 on failure
int del_file(struct ext2_inode *inode, int offset, char *file) {
	struct ext2_inode *parentdir = inode + offset;
    char name[EXT2_BLOCK_SIZE];
    int x, y, z;
    for (x = 0; x < 12 && parentdir->i_block[x]; x++) {
		y = 0;
        while(y < EXT2_BLOCK_SIZE) {
            struct ext2_dir_entry_2 *de = (void *)(disk + (EXT2_BLOCK_SIZE * parentdir->i_block[x]) + y);
            memcpy(name, de->name, de->name_len);
            name[de->name_len] = 0;
            // File is found
            if (!strncmp(file, name, de->name_len)) {
                // Get the inode of the file to remove
                struct ext2_inode *target = inode + (de->inode - 1);
                // File was previously removed but exists in disk
                if (!de->inode) {
                    return -1;
                }
                // Error if the file is not a regular file
                if (de->file_type != 1) {
                    return -2;
                }
                memset(de, 0, sizeof(struct ext2_dir_entry_2));
                target->i_links_count--; // Decrement link_count of specific inode
                // No more links
                if (!target->i_links_count) {
                    struct ext2_super_block *super_block = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
                    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE * 2));
                    unsigned int *inode_bitmap = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * group_desc->bg_inode_bitmap ));
                    unsigned int *block_bitmap = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * group_desc->bg_block_bitmap));
                    bitmap_rm (inode_bitmap, de->inode);
                    super_block->s_free_inodes_count++; // Increment free inode count
                    for (z = 0; z < 12 && target->i_block[z]; z++) {
                        bitmap_rm (block_bitmap, target->i_block[z]);
                        super_block->s_free_blocks_count++; // Increment free blocks
                    }
                    // There is an indirect block
                    if (target->i_block[x] && x == 12) {
                        // Theres still more space to remove
                        unsigned int *indirect_location = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * target->i_block[12]));
                        // Keep on removing indirect blocks
                        for (z = 0; indirect_location[z]; z++) {
                            bitmap_rm (block_bitmap, indirect_location[z]);
                        }
                        bitmap_rm (block_bitmap, target->i_block[12]); // Clear the indirect block pointer
                    }
                }
                target = 0; // Target "no longer" exists
                return 0;
            }
            if(!de->rec_len) break; // Next dir entry is null
            y+= de->rec_len;
            de += de->rec_len;
        }
    }
    return -1; // Returns -1 if the file was not found
}

// Clears the bitmap at a location
void bitmap_rm (unsigned int *bitmap, int location) {
    // Unset the bitmap at the current location
    bitmap[(location - 1) / 8] &= ~(1 << ((location - 1) % 8));
}

// Gets the inode of a file
int get_file_inode (struct ext2_inode *inode, char *filename) {
    int x, y;
    char name[EXT2_BLOCK_SIZE];
    // Find if the file already exists
    for (x = 0; x < 12 && inode->i_block[x]; x++) {
		y = 0;
        while(y < EXT2_BLOCK_SIZE) {
            struct ext2_dir_entry_2 *de = (void *)(disk + (EXT2_BLOCK_SIZE * inode->i_block[x]) + y);
            memcpy(name, de->name, de->name_len);
            name[de->name_len] = 0;
            // File specified exists in destination directory
            if (!strncmp(filename, name, de->name_len)) {
                return de->inode-1;
            }
            if(!de->rec_len) break; // Next dir entry is null
            y+= de->rec_len;
            de += de->rec_len;
        }
    }
    return -1;
}

// Adds a new bitmap
void add_bitmap (unsigned int *bitmap, int location) {
    bitmap[(location - 1) / 8] |= (1 << ((location - 1) % 8));
}
// Returns the location of a free inode, returns -1 on failure
unsigned int get_free_inode () {
    struct ext2_super_block *super_block = (void *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *group_desc = (void *)(disk + (EXT2_BLOCK_SIZE * 2));
    unsigned char *inode_bitmap = (unsigned char *)(disk + (EXT2_BLOCK_SIZE * group_desc->bg_inode_bitmap));
    int displace = 1;
    int shift;
    // Disk isn't full
    if (super_block->s_free_inodes_count != 0) {
        shift = (EXT2_GOOD_OLD_FIRST_INO - 1) % 8;
        // Cycle all the inodes
        while (displace < (super_block->s_inodes_count/8)) {
            for(;shift < 8; shift++) {
                // Returns a free one
                if (((inode_bitmap[displace] >> shift) & 1) == 0) {
                    return (displace * 8) + shift + 1;
                }
            }
            shift = 0;
            displace++;
        }
    }
    return -1;
}

// Finds a free block on the block bitmap, returns -1 on failure
unsigned int get_free_block () {
    struct ext2_super_block *super_block = (void *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *group_desc = (void *)(disk + (EXT2_BLOCK_SIZE * 2));
    unsigned int *block_bitmap = (unsigned int *)(disk + (EXT2_BLOCK_SIZE * group_desc->bg_block_bitmap));
    int displace = 0;
    int shift;
    // Disk isn't full
    if (super_block->s_free_blocks_count != 0) {
        // Cycle all the inodes
        while (displace < (super_block->s_blocks_count)) {
            for(shift = 0; shift < 8; shift++) {
                // Found a non occupied block
                if (((block_bitmap[displace] >> shift) & 1) == 0) {
                    return (displace * 8) + shift + 1;
                }
            }
            displace++;
        }
    }
    return -1;
}
