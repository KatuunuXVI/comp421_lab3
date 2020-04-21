# comp421_lab3

## Notes
### Header
* BLOCKSIZE is equal to SECTORSIZE, Only one Block per Sector
* Boot Block is not used by file system, so 0 can function as a null pointer for block numbers
* Block 1 possesses the fs_header, which provides general information about the file system such as:
    * Total blocks in the system
    * Number of inodes in system
    * Replaces first inode of block 1, thus has padding to take up same size as an inode
* num_inodes does not count header
* Inodes must not be split by blocks
* fs_header takes place of inode 0, first valid inode is inode 1
* Each block has BLOCKSIZE/INODESIZE inodes, except for block 1

###Inodes
* <em>type</em>
    * INODE_FREE - Not in use for any file
    * INODE_DIRECTORY - Refers to a directory
    * INODE_REGULAR - Refers to a regular data file
    * INODE_SYMLINK - Refers to a symbolic link
* <em>nlink</em> - Number of hrd links that refer to this file or directory
* <em>reuse</em> - Number of times Inode has been reused. Initialized to 0 when file system is formatted. Increments each time Inode is allocated
* <em>size</em> - File Size in bytes. Includes ALL allocated space for the file, even unused
* <em>direct</em> - Array that specifies the numbers of the first NUM_DIRECT blocks of the file.
* <em>indirect</em>
    * Number of the block for a file, specified for formatting the remaining blocks of the file, past NUM_DIRECT
    * Has an array of block numbers pointing to the last blocks where the file is contained.
    * Only one can be used for every file. Extending a file past this point should result in an error

###Data Blocks and Directories
* Defined by <em>num_blocks</em> - <em>num_inodes</em>
* Root directory refered to by ROOTINODE
* Directories cannot be directly written to by user programs, though they can be read by them
* Links cannot point to directories, thus the file structure prohibits loops.
* Directory entry struct contains:
    * <em>inum</em> - Inode number mapped to this directory. If this field is unused it should be 0, and should be altered to such when necessary.
    * <em>name</em> - A array of exactly DIRNAMELEN characters, component of the filepath. Can contain any character except foreslash and null, though the latter should be used if the actual name is less than DIRNAMELEN.
   
## To Do List