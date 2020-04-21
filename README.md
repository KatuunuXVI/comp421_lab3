# comp421_lab3

## Notes
*Header
* BLOCKSIZE is equal to SECTORSIZE, Only one Block per Sector
* Boot Block is not used by file system, so 0 can function as a null pointer for block numbers
* Block 1 possesses the fs_header, which provides general information about the file system such as:
** Total blocks in the system
** Number of inodes in system
** Replaces first inode of block 1, thus has padding to take up same size as an inode
* num_inodes does not count header
* Inodes must not be split by blocks
* fs_header takes place of inode 0, first valid inode is inode 1
* Each block has BLOCKSIZE/INODESIZE inodes, except for block 1

*Inodes
* TYPE
** INODE_FREE - Not in use for any file
** INODE_DIRECTORY - Refers to a directory
** INODE_REGULAR - Refers to a regular data file
** INODE_SYMLINK - Refers to a symbolic link
* NLINK - Number of hrd links that refer to this file or directory
* REUSE - Number of times Inode has been reused. Initialized to 0 when file system is formatted. Increments each time Inode is allocated
* SIZE - File Size in bytes. Includes ALL allocated space for the file, even unused
* DIRECT - Array that specifies the numbers of the first NUM_DIRECT blocks of the file.
* INDIRECT  
** Number of the block for a file, specified for formatting the remaining blocks of the file, past NUM_DIRECT
** 



## To Do List