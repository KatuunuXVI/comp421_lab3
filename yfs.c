#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include "yfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    printf("Argument Count: %d\n",argc);
    int i;
    for(i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
    Register(FILE_SERVER);
    struct fs_header *header;
    /** Obtain File System Header */
    char sector_one[SECTORSIZE];
    if(ReadSector(1,sector_one) == 0) {
        header = (struct fs_header *)sector_one;
        printf("Char Size: %d\n", (int) sizeof(char));
        printf("Sector Size: %d\n",SECTORSIZE);
        printf("Num Blocks: %d\n",header->num_inodes);
    } else {
        printf("Error\n");
    }

    /**Check Inodes */
    printf("INODE SIZE: %d\n",INODESIZE);
    printf("Inode Struct Size %d\n", (int) sizeof(struct inode));
    struct inode *in1 = (struct inode *)header + 1;
    printf("Inode Reuse: %d\n",in1->size);

    return 0;
};

//struct inode* get_inode(int inode_num) {
  //  printf()
//}