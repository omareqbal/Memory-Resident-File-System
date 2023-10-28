# Memory Resident File System

Implementation of a memory-resident file system in which the disk is simulated as a set of blocks in memory.

### Alternative 1 - Linked List Implementation using FAT

The data structures used are - 

Block0 - It is the super block which stores block_size, total_size, bit vector for maintaining the free blocks and pointer to the block storing the directory

Block1- It stores the FAT

directory - It stores the file name and starting block for each file

Other - It is the data block containing a char array of size block_size

FD - File Descriptor Table for maintaining information about currently opened files


### Alternative 2 - Indexed implementation using i-node

The data structures used are - 

Block0 - It is the super block which stores block_size, total_size, bit vector for maintaining the free inodes and pointer to the first free block

Block12 - It stores the Inode which has the following fields -
		name[14] - file name
		type - DIRECTORY or REGULAR
		size - file size
		dp[5] - array of direct pointers
		sip - singly indirect pointer
		dip - doubly indirect pointer

directory - It stores the file name and inode no for each file

Other - It is the data block containing a char array of size block_size

FD - File Descriptor Table for maintaining information about currently opened files


### Functions implemented for both the parts are - 

initialize - It initializes all the structures based on the total size and block size.

my_open - It opens a file in read or write mode and inserts an entry into File Descriptor Table. If file does not exist and it is opened in write mode, then new file will be created.

my_read - It reads from an open file.

my_write - It writes to an open file.

my_close - It closes the file and marks the entry in File Descriptor Table as invalid.

my_cat - It displays the contents of a file.

my_copy - It copies the contents of first file into second file. One of the files is of Linux file system and the other is of implemented file system. It takes an argument file1_type which indicates whether the first file is of Linux file system or of implemented file system.

### Functions implemented for only the 2nd part - mkdir, chdir and rmdir

my_mkdir - It creates a new directory

my_chdir - It changes the working directory

my_rmdir - It removes a directory along with all its contents
