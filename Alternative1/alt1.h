#ifndef _ALT_1_H
#define _ALT_1_H
#include<bits/stdc++.h>
#include<unistd.h>
#include<fcntl.h>

#define MAX_FILENAME 100
#define RDONLY 1
#define WRONLY 2
#define RDWR 3
#define LINUX_FILE 0
#define ALT1_FILE 1

using namespace std;

typedef struct{
	char filename[MAX_FILENAME];
	int blockno;	//starting block number
	}directory;

typedef struct{
	int block_size;
	int tot_size;
	int nblocks;
	char name[20];
	int *freeblocks;	//bit vector for storing the free blocks
	directory *dir;		//pointer to the block storing the directory
	}Block0;

typedef struct{
	int *next;			//pointer to FAT
	}Block1;

typedef struct{
	char *contents;		//content of data blocks
	}Other;

typedef struct{
	Block0 super;
	Block1 FAT;
	directory *block2;
	Other *blocks;
	}Block;

typedef struct{
	directory *file;
	int cur_block;
	int offset;
	int option;
	int valid;
}FD;	//file descriptor table

int my_open(const char *filename, int option);
int my_read(int fd, char *buf, int len);
int my_write(int fd,const char *buf, int len);
int my_close(int fd);
int initialize(int totalsize, int blocksize);
int my_cat(const char *filename);
int my_copy(const char *filename1, const char * filename2, int file1_type);

#endif
