# Comp 421 Lab 3

## Project Participants

- Jungwoo Lee (jl40)
- Nathan Eigbe (nee2)

## Test Cases

- [x] sample1.c
  - Create, Write, Close 6 files
  - Running it once will create new files
  - Running it twice will truncate existing files with new blocks
- [x] sample2.c
  - Create and Close 32 empty files
  - Running it twice will truncate but will not free any blockst
- [x] tcreate.c
  - Create, Sync, Delay
- [x] tcreate2.c
  - Create 2 files, overwrite 1 file, last one is invalid.
- [x] topen2.c
  - Open 4 files created from tcreate2.c
- [x] tlink.c
  - Create and Link "/a" with "/b"
  - Running it twice will not create a link
- [x] tls.c
  - ChDir, Open, Read, Stat, and ReadLink
- [ ] tsymlink.c
  - Nope
- [x] tunlink2.c
  - Unlink 4 files created from tcreate2
- [x] writeread.c
  - Create, Write, Close, Sync, Open, Read, and Close

## Additional Test Cases

- [x] tseek.c
  - Test SEEK_SET and SEEK_CUR
- [x] treuse.c
  - New create will take spot that was unlinked
- [x] tdirsize.c
  - Size of root directory goes back to 64 after last dir is unlinked
- [x] thole1.c
  - Create a hole for 2nd block with Write
- [x] trmdir1.c
  - Create a dir, remove dir with file and without file
- [x] trmdir2.c
  - Create a dir, chdir in there, remove that dir, and create relative file (should fail).
- [x] tindirect1.c
  - Create a file which writes in first block and seek beyond direct to write indirect blocks. Then unlink it to see if it frees all relevant blocks.
