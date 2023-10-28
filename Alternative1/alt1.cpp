#include "alt1.h"

Block fsystem;
vector<FD> fdt;
int MAX_FILES;

int my_open(const char *filename, int option){
	directory* d=fsystem.super.dir;
	int i,j,k,freeblockno,next;

	//search the file in the directory
	for(i=0;i<MAX_FILES;i++){
		if(d[i].blockno==-1)
			break;
		if(strcmp(filename,d[i].filename)==0){
			//file exists
			FD a;
			//create an entry to be inserted in the File Descriptor Table
			a.file=&d[i];
			a.option=option;
			a.cur_block=d[i].blockno;
			a.offset=0;
			a.valid=1;
			if(option==WRONLY){
				//if opened in write mode, clear and free the used blocks
				next=fsystem.FAT.next[a.cur_block];
				memset(fsystem.blocks[a.cur_block-3].contents, 0, fsystem.super.block_size);
				while(next!=-1){
					fsystem.super.freeblocks[next]=0;
					memset(fsystem.blocks[next-3].contents, 0, fsystem.super.block_size);
					next=fsystem.FAT.next[next];
				}
				fsystem.FAT.next[a.cur_block]=-1;
			}
			fdt.push_back(a);
			return fdt.size()-1;	//return the index of FDT entry
		}
	}

	//file does not exist
	if(i==MAX_FILES)
		return -1;
	if(option!=WRONLY)
		return -1;
	else{
		//if opened in write mode, allocate a new block
		for(k=0;k<fsystem.super.nblocks;k++){
			if(fsystem.super.freeblocks[k]==0){
				freeblockno=k;
				fsystem.super.freeblocks[k]=1;
				break;
			}
		}
		if(k==fsystem.super.nblocks)
			return -1;
		
		strcpy(d[i].filename,filename);
		//update the directory and FAT
		d[i].blockno=freeblockno;
		fsystem.FAT.next[freeblockno]=-1;

		//insert in the FDT
		FD a;
		a.file=&d[i];
		a.option=option;
		a.cur_block=freeblockno;
		a.offset=0;
		a.valid=1;
		fdt.push_back(a);
		return fdt.size()-1;	//return the index of FDT entry
	}
	return -1;
}

int my_read(int fd, char *buf, int len){
	if(fd>=fdt.size() || fdt[fd].valid==0 || (fdt[fd].option!=RDONLY&&fdt[fd].option!=RDWR))
		return -1;
	if(fdt[fd].cur_block==-1)
		return -1;
	FD a = fdt[fd];
	int char_count=0,j;

	//read from the current block
	for(j=a.offset;j<fsystem.super.block_size && char_count<len;j++){
		buf[char_count++]=fsystem.blocks[a.cur_block-3].contents[j];
		if(fsystem.blocks[a.cur_block-3].contents[j]=='\0')
			break;
	}
	a.offset=j%fsystem.super.block_size;
	//if len number of characters have been read, return
	if(char_count==len || j<fsystem.super.block_size){
		if(j==fsystem.super.block_size)
			a.cur_block=fsystem.FAT.next[a.cur_block];
		fdt[fd]=a;
		return char_count;
	}

	//read from the next block, until either len number of characters are read
	//or all blocks are read
	a.cur_block=fsystem.FAT.next[a.cur_block];

	while(a.cur_block!=-1 && char_count<len){
		for(j=0;j<fsystem.super.block_size && char_count<len; j++){
			buf[char_count++]=fsystem.blocks[a.cur_block-3].contents[j];
			if(fsystem.blocks[a.cur_block-3].contents[j]=='\0'){
				a.offset=j;
				fdt[fd]=a;
				return char_count;
			}
		}
		if(j==fsystem.super.block_size){
			a.offset=0;
			a.cur_block=fsystem.FAT.next[a.cur_block];
		}
		else{
			a.offset=j;
			fdt[fd]=a;
			return char_count;
		}
	}
	fdt[fd]=a;
	return char_count;
}

int my_write(int fd,const char *buf, int len){
	if(fd>=fdt.size() || fdt[fd].valid==0 || (fdt[fd].option!=WRONLY&&fdt[fd].option!=RDWR))
		return -1;
	if(len==0)
		return 0;
	int curno,i,j,k,freeblockno,char_count=0;
	FD a=fdt[fd];
	if(a.cur_block != -1){
		//if there is space for more characters in current block
		//write upto the end of this block
		for(j=a.offset;j<fsystem.super.block_size && char_count<len;j++){
			fsystem.blocks[a.cur_block-3].contents[j]=buf[char_count++];
		}
		if(char_count==len){
			//if all characters are written, return
			if(j==fsystem.super.block_size){
				a.cur_block=-1;
				a.offset=0;
				fdt[fd]=a;
				return char_count;
			}
			else{
				a.offset=j;
				fdt[fd]=a;
				return char_count;
			}
		}
	}
		
	while(char_count < len){
		//allocate new block, and write to the end of this block
		for(k=0;k<fsystem.super.nblocks;k++){
			if(fsystem.super.freeblocks[k]==0){
				freeblockno=k;
				fsystem.super.freeblocks[k]=1;
				break;
			}
		}
		if(k==fsystem.super.nblocks)
			return (char_count==0)?-1:char_count;
		curno=a.file->blockno;
		while(fsystem.FAT.next[curno] != -1){
			curno=fsystem.FAT.next[curno];
		}
		//update FAT for new block allocated
		fsystem.FAT.next[curno]=freeblockno;
		fsystem.FAT.next[freeblockno]=-1;

		for(j=0;j<fsystem.super.block_size && char_count<len;j++){
			fsystem.blocks[freeblockno-3].contents[j]=buf[char_count++];
		}
		if(char_count==len){
			if(j==fsystem.super.block_size){
				a.cur_block=-1;
				a.offset=0;
				fdt[fd]=a;
				return char_count;
			}
			else{
				a.cur_block=freeblockno;
				a.offset=j;
				fdt[fd]=a;
				return char_count;
			}
		}
	}
	return -1;
}

int my_cat(const char *filename){
	int i,n;
	int fd=my_open(filename,RDONLY);
	if(fd==-1)	//file does not exist
		return -1;
	char buf[20];
	//read upto end of file and print
	while(1){
		n=my_read(fd,buf,10);
		for(i=0;i<n;i++){
			cout<<buf[i];
		}
		if(buf[n-1]=='\0')
			break;
	}
	cout<<endl;
	my_close(fd);
	return 0;

}

int my_copy(const char *filename1, const char * filename2, int file1_type){
	int from_fd,to_fd,n,m;
	char buf[20];
	if(file1_type==LINUX_FILE){
		//if 1st file is of Linux file system
		from_fd=open(filename1,O_RDONLY);
		to_fd=my_open(filename2,WRONLY);
		if(from_fd==-1 || to_fd==-1)
			return -1;
		while(1){
			n=read(from_fd,buf,20);
			m=my_write(to_fd,buf,n);
			if(m==-1) return -1;
			if(buf[n-1]=='\0')
				break;
		}	
		close(from_fd);
		my_close(to_fd);
	}
	else{
		from_fd=my_open(filename1,RDONLY);
		to_fd=open(filename2,O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if(from_fd==-1 || to_fd==-1)
			return -1;
		while(1){
			n=my_read(from_fd,buf,1);
			if(buf[0]=='\0')
				break;
			m=write(to_fd,buf,1);
			if(m==-1) return -1;
		}
		my_close(from_fd);
		close(to_fd);
	}
	return 0;
}
int my_close(int fd){
	if(fd>=fdt.size() || fdt[fd].valid==0)
		return -1;
	fdt[fd].valid=0;
	return 0;
}

int initialize(int totalsize, int blocksize){
	int nblocks=totalsize/blocksize; 
	if(sizeof(Block0)+nblocks*sizeof(int)>blocksize || sizeof(Block1)+nblocks*sizeof(int)>blocksize)
		{
			cout<<"Invalid Input: Problem with Superblock or FAT Allocation \n";
			return -1;
		}
	int i;
	MAX_FILES=blocksize/sizeof(directory);
	fsystem.super.block_size=blocksize;
	fsystem.super.tot_size=totalsize;
	fsystem.super.nblocks=nblocks;
	strcpy(fsystem.super.name,"Alternative1");
	
	//allocate memory for bit vector maintaining the free blocks
	//block 0,1,2 are not free, rest are free initially
	fsystem.super.freeblocks=new int[nblocks];
	fsystem.super.freeblocks[0]=fsystem.super.freeblocks[1]=fsystem.super.freeblocks[2]=1;
	for(i=3;i<nblocks;i++) fsystem.super.freeblocks[i]=0;
	
	fsystem.block2=new directory[MAX_FILES];	//block2 stores the directory
	fsystem.super.dir=fsystem.block2;	//pointer to block2 stored in super block
	
	for(i=0;i<MAX_FILES;i++) fsystem.block2[i].blockno=-1;

	fsystem.FAT.next=new int[nblocks];	//allocate memory for FAT
	
	//allocate memory for the date blocks
	fsystem.blocks=new Other[nblocks-3];
	for(i=0;i<nblocks-3;i++) 
		{
			fsystem.blocks[i].contents=new char[blocksize];
		}
	return 0;

}

