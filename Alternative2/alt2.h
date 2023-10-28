#ifndef _ALT2_H
#define _ALT2_H

#include<bits/stdc++.h>
#include<unistd.h>
#include<fcntl.h>

#define RDONLY 1
#define WRONLY 2
#define RDWR 3

#define DIRECTORY 0
#define REGULAR 1
#define LINUX_FILE 0
#define ALT2_FILE 1

using namespace std;

typedef struct{					//Directory structure, 14 bytes for filename and 2 for inode number
	char fname[14];
	short inodeno;
}directory;

typedef struct{					//Super block with relevant details
	int block_size;
	int tot_size;
	int nblocks;
	int ninodes;
	char name[20];
	int ffb;					//First free block
	int *freeinodes;
	}Block0;

typedef struct{					//Inode structure. Has the name, type, size, 5 direct ptrs and 1 Singly and 1 doubly indirect pointer
	char name[14];
	int type;
	int size;
	int dp[5];
	int sip;
	int dip;
	}Inode;

typedef struct{					//Block 1 and Block 2 are made of inodes
	Inode *inode_list;
}Block12;

typedef struct{
	char *contents;				//content of data blocks
}Other;

typedef struct{					//Full system
	Block0 super;
	Block12 ilist;
	Other *blocks;
	}Block;

typedef struct{					//File descriptor stores the corresponding inode, the current position in dip, sip and dp along with offset in block etc.
	Inode *file;
	int cur_dip;
	int cur_sip;
	int cur_dp;
	int offset;
	int option;
	int valid;
}FD;	

int initialize(int totalsize, int blocksize);
int my_open(const char *filename, int option);
int my_read(int fd, char *buf, int len);
int my_write(int fd, const char *buf, int len);
int my_close(int fd);
int my_mkdir(const char * dirname);
int my_chdir(const char* dirname);
int my_rmdir(const char * dirname);
int my_copy(const char *filename1, const char * filename2, int file1_type);
int my_cat(const char *filename);
#endif