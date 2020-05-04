# comp421_lab3

## Variables

- BLOCKSIZE/SECTORSIZE: 512
- NUMBLOCKS/NUMSECTORS: 1426
- INODESIZE: 64
- NUM_DIRECT: 12
- MAXPATHNAMELEN: 256
- MAXSYMLINKS: 20
- BLOCK_CACHESIZE: 32
- INODE_CACHESIZE: 16
- MAX_OPEN_FILES: 16
- FILE_SERVER: 1

## Test Cases

- [x] sample1.c
  - Create, Write, Close 6 files
- [x] sample2.c
  - Create and Close 32 empty files
- [ ] tcreate.c
  - Create, Sync, Delay
- [ ] tcreate2.c
  - Create 4 files
- [ ] topen2.c
  - Open 4 files created from tcreate2.c
- [ ] tlink.c
  - Create and Link "/a" with "/b"
- [ ] tls.c
  - ChDir, Open, Read, Stat, and ReadLink
- [ ] tsymlink.c
  - Create, SymLink, ReadLink, Stat and more Symlink
- [ ] tunlink2.c
  - Unlink 4 files
- [ ] writeread.c
  - Create, Write, Close, Sync, Open, Read, and Close

## Additional Test Cases

- [ ] tseek.c
  - Test SEEK_SET and SEEK_CUR
- [ ] Test reuse
- [ ] Test creating block holes
- [ ] Test seek
- [ ] Test indirect block
  > > > > > > > master

## Notes

### Header

- BLOCKSIZE is equal to SECTORSIZE, Only one Block per Sector
- Boot Block is not used by file system, so 0 can function as a null pointer for block numbers
- Block 1 possesses the fs_header, which provides general information about the file system such as:
  - Total blocks in the system
  - Number of inodes in system
  - Replaces first inode of block 1, thus has padding to take up same size as an inode
- num_inodes does not count header
- Inodes must not be split by blocks
- fs_header takes place of inode 0, first valid inode is inode 1
- Each block has BLOCKSIZE/INODESIZE inodes, except for block 1

###Inodes

- <em>type</em>
  - INODE_FREE - Not in use for any file
  - INODE_DIRECTORY - Refers to a directory
  - INODE_REGULAR - Refers to a regular data file
  - INODE_SYMLINK - Refers to a symbolic link
- <em>nlink</em> - Number of hard links that refer to this file or directory
- <em>reuse</em> - Number of times Inode has been reused. Initialized to 0 when file system is formatted. Increments each time Inode is allocated
- <em>size</em> - File Size in bytes. Includes ALL allocated space for the file, even unused
- <em>direct</em> - Array that specifies the numbers of the first NUM_DIRECT blocks of the file.
- <em>indirect</em>
  - Number of the block for a file, specified for formatting the remaining blocks of the file, past NUM_DIRECT
  - Has an array of block numbers pointing to the last blocks where the file is contained.
  - Only one can be used for every file. Extending a file past this point should result in an error

### Data Blocks and Directories

- Defined by <em>num_blocks</em> - <em>num_inodes</em>
- Root directory refered to by ROOTINODE
- Directories cannot be directly written to by user programs, though they can be read by them
- Links cannot point to directories, thus the file structure prohibits loops.
- Directory entry struct contains:
  - <em>inum</em> - Inode number mapped to this directory. If this field is unused it should be 0, and should be altered to such when necessary.
  - <em>name</em> - A array of exactly DIRNAMELEN characters, component of the filepath. Can contain any character except foreslash and null, though the latter should be used if the actual name is less than DIRNAMELEN.
- Slash at the beginning of a filepath indicates it is an absolute path, rather than a relative path.
- Multiple consecutive slashes in a path don't make any difference
- During an Open Operation, each component of the slash should be processed individually
- Each directory has two special directory entries:
  - '.' - Has name, '.', and the inode of the directory
  - '..' - Has name, '..', and the inode of the parent directory.
  - For the root directory, '.' and '..' are identical.

### Symbolic Links

- A symbolic link is similar to a regular file, but it's contents are entirely the filename of the file it links to.
- It may point to an absolute or relative filepath.
- When opening a symbolic link, if you encounter another symbolic link, you should recurse.
- Though it is not an error to link to a non-existant target, it is to link to an empty filepath. That said, if you open a symbolic link that leads to a non-existant or deleted target, it should trigger an error.
- You cannot create a hard link to a directory, though you can create a symbolic link to one.
- If more than MAXSYMLINKS are encountered in a filepath, the Open process should be terminated and return an error.
- If a symbolic link is in

### Yalnix Kernel Calls

- **int ReadSector(int sectornum, void \*buf)** - Reads <em>sectornum</em> into the address at buf. <em>buf</em> must be SECTORSIZE bytes. Calling process is blocked until the process completes. Returns 0 if it completes correctly, ERROR if <em>sectornum</em> is invalid or <em>buf</em> cannot be written to.
- **int WriteSector(int sectornum, void \*buf)** - Same as **ReadSector** but writes into the sector, rather than copying from.
- **int Register(unsigned int service_id)** - Registers the calling process as the provider of a particular service, identified by <em>service_id</em>. Process is then considered the server for that service. Returns 0 on success, ERROR if another process is already registered for this service.
- **int Send(void \*msg, int pid)** - <em>msg</em> pointer to a 32 byte sized buffer. Holding a message to be sent. Sends message to the process determined by <em>pid</em>, though it can also be the negative value of the <em>service_id</em>. If the process identified by <em>pid</em> exits before it calls Reply, ERROR is returned.
- **int Receive(void \*msg)** - Receives the <em>msg</em> that another process has sent. If nothing has been sent, blocks until a mesage is. If every process is blocked on **Receive** however, the kernel should have each automatically return 0, so to avoid deadlock.
- **int Reply(void \*msg)** - Sends a reply to a process blocked by a send call. <em>msg</em> overwrites the address space of the original message of the sender.
- **int CopyFrom(int srcpid, void _dest, void src, int len)_ / int CopyFrom(int srcpid, void \*dest, void src, int len)** - Can only be called between a receive and reply. Used to transfer data between virtual space of two communicating processes
- C compiler will add padding to structs to avoid

###Yalnix File System Server

- Yalnix makes requests to the server through stubs of the arguments through the 'Send' call
- When 'Send' is called, the calling process blocks until it receives the 'Reply' return value
- Server has no knowledge of open files, such knowledge is held by the processes calling the server.
- The server has a cache of recently accessed blocks of size BLOCK_CACHESIZE. A cache of recently accessed inodes of size INODE_CACHESIZE also exists.

### File System Library

- Meant to record open files or directories by processes
- Cannot Create() or Open() more than MAX_OPEN_FILES files
- Maintains an <em>open file table</em> for keeping track of open files. Can be represented by an array of pointers to each file's data structure representation.
- Index in this array serves as file descriptor

### File System Calls

- **int Open(char \*pathname)** - Used to open the string of pathname.Returns a fd integer, for future uses for referring to the opened file. File descriptor must be lowest possible value.

- **int Close(int fd)** - Closes the file refered to by <em>fd</em>
- **int Create(char \*pathname)** - Creates a file at the specified path and then opens it. Any directory in the path must already exist. If the file exists, then the file is replaced by an empty file of size zero, though this is an error if it is attempted on a directory. Returns the file descriptor for the created file.
- **int Read(int fd, void \*buf, int size)** - Reads <em>size</em> bytes from the file specified by <em>fd</em> into the address pointed to by <em>buf</em>. Initial position in file is 0 after **Open**, though the position increments after each read depending on how many bytes are read. Returns number of bytes read.
- **int Write(int fd, void \*buf, int size)** - Same as read, but copies from <em>buf</em> rather than to buf.
- **int Seek(int fd, int offset, int whence)** - Simply changes the position of the file by offset. Whence determines from where it travels:
  _ SEEK_SET - Beginning of the file
  _ SEEK_CUR - Current location of the file \* SEEK_END - End of the file
  Traveling before the start of the file results in an error. Returns the new position. Traveling beyonf the end of the file, creates allowance for if the files size is increased.
- **int Link(char* oldname, char* newname)** - Creates a link from <em>newname</em> to <em>oldname</em>. <em>oldname</em> can't be a directory, and the two files can't be in the same directory.
- **int Unlink(char \* pathname)**- Removes the directory entry for <em>pathname</em>. If this is the last link to a file, it should be deleted and it's inode freed. Must not be a directory.
- **int MkDir(char \* pathname)** - Creates a new directory based on <em>pathname</em>. Must not already exist.
- **int RmDir(char \* pathname)** - Removes a directory and every file inside. Must not contain any directories. Root directory cannot be removed.
- **int ChDir(char \* pathname)** - Changes the current directory of a process by returning the inode of <em>pathname</em>. <em>pathname</em> must refer to a directory.
- **int Stat(char _ pathname, struct Stat _ statbuf)** - Returns information about the file at <em>pathname</em> to the struct at <em>statbuf</em>.
- **int Sync(void)** - Writes all dirty cached inodes back to their corresponding disk blocks, and the dirty cached disk blocks back to the disk.
- **int Shutdown(void)** - Syncs the cache, and then calls the Yalnix Exit.

## To Do List

- [x] Obtain Header
- [x] Obtain Root Inode
- [x] Implement stack based caches for inodes and blocks
  - [x] Implement stack search
  - [x] Implement stack entry raise
- [x] Get block or inode by number
- [x] Get list of free inodes
- [ ] Get list of free blocks
- [ ] Fork and Execute argument process
