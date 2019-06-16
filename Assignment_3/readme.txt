ext2.h: existing headers
ext2_ls.c: complete functionality implemented
ext2_cp.c: complete functionality implemented although chcecking for a copied
    file with the ext2_ls will not show the file but mounting the image will 
    show the file.
    NOTE: cp returns an error if you try to cp to an already existing file
        it does not overwrite the file
    You can specify a destination name for the file being copied but if you
        don't, it defaults to file being copied from.
ext2_mkdir.c: error checking and basic information, rest of implementation not completed
ext2_ln.c: error checking and functionality works, once the current block is full there will be no more room for links
ext2_rm.c: complete functionality implemented, you cannot remove directories
    and if you try, it will result in errors
ext2_utils.h: contains the headers for the util functions
ext2_utils.c: contains all of the util functions we needed in this assignment
Makefile: makefile complete, running make will compile all of the ext2 functions in this assignment
