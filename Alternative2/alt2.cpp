#include "alt2.h"

Block fsystem;					//File system
vector<FD> fdt;					//File description table
int cur_dir;					//Current directory

int searchDP_Directory(const char *filename,int blockno)			//To search for a filename in a given directory block
{
	directory *buf=(directory *)fsystem.blocks[blockno-3].contents;	//Get the contents
	int i;

	for(i=0;i<fsystem.super.block_size/sizeof(directory);i++){
		if(buf[i].inodeno==-1) return -1;							//Search iteratively till you get the right inode number
		if(strcmp(filename,buf[i].fname)==0)
			return buf[i].inodeno;
	}
	return -1;
}

int searchDP_Directory_WR(const char *filename,int blockno)			//Search for filename in a given directory when asked to open in WRONLY mode. Here we create file if it does not exist.
{
	directory *buf=(directory *)fsystem.blocks[blockno-3].contents;
	int i,j,inodeno,fileblock;
	Inode *fileinode;
	for(i=0;i<fsystem.super.block_size/sizeof(directory);i++){
		if(buf[i].inodeno==-1)										//If we reach the first empty block means search unsuccessful
		{
			fileblock=fsystem.super.ffb;
			fsystem.super.ffb=*((int*)fsystem.blocks[fsystem.super.ffb-3].contents);
			memset(fsystem.blocks[fileblock-3].contents, 0, fsystem.super.block_size);
			for(j=0;j<fsystem.super.ninodes;j++){					//Get a free block and free inode and allocate it to file
				if(fsystem.super.freeinodes[j]==0){
					inodeno=j;
					fileinode=&fsystem.ilist.inode_list[j];
					strcpy(fileinode->name,filename);
					fileinode->type=REGULAR;
					fileinode->size=0;
					fileinode->dp[0]=fileblock;
					fsystem.super.freeinodes[j]=1;
					break;
				}
			}
			if(j==fsystem.super.ninodes)							//If the directory is full and element is not found
				return -1;

			strcpy(buf[i].fname,filename);
			buf[i].inodeno=inodeno;
			fsystem.ilist.inode_list[cur_dir].size++;
			return inodeno;							//Return the new file inode
		}
		if(strcmp(filename,buf[i].fname)==0)				//If the file is found
			return buf[i].inodeno;
	}
	return -1;
}

void clearSIP(int blockno){					//To clear the SIP of an inode
	int* a=(int*)fsystem.blocks[blockno-3].contents;
	int i,j;
	for(i=0;i<fsystem.super.block_size/sizeof(int);i++){
		if(a[i]==-1)						//Clear the individually pointing blocks and return
			return;
		memset(fsystem.blocks[a[i]-3].contents, 0, fsystem.super.block_size);
		*((int*)fsystem.blocks[a[i]-3].contents)=fsystem.super.ffb;
		fsystem.super.ffb=a[i];
	}
}

void clearDIP(int blockno){					//To clear the DIP of an inode
	int* a=(int*)fsystem.blocks[blockno-3].contents;
	int i,j;
	for(i=0;i<fsystem.super.block_size/sizeof(int);i++){
		if(a[i]==-1)						//Clear the individual pointing SIPs and return
			return;
		clearSIP(a[i]);
		memset(fsystem.blocks[a[i]-3].contents, 0, fsystem.super.block_size);
		*((int*)fsystem.blocks[a[i]-3].contents)=fsystem.super.ffb;
		fsystem.super.ffb=a[i];
	}
}

int my_open(const char *filename, int option)		//open a new file with possible options as WRONLY/RDONLY/RDWR
{
	int i,j,inodeno,fileblock,dirfreeblock,blockno;
	for(i=0;filename[i]!='\0';i++)
	{
		if(filename[i]=='/') 
			return -1;
	}

	Inode cwd=fsystem.ilist.inode_list[cur_dir];	//Get the inode of current working directory
	Inode* fileinode;
	if(cwd.type!=DIRECTORY) return -1;
	if(option==RDONLY || option==RDWR)				//If option is RDONLY or RDWR
	{
		for(i=0;i<5;i++){
			if(cwd.dp[i]==-1)						
				return -1;
			inodeno=searchDP_Directory(filename,cwd.dp[i]);	//Search the direct pointers of the directory
			if(inodeno!=-1){						//if found
				FD a;
				a.file=&fsystem.ilist.inode_list[inodeno];
				a.cur_dp=0;
				a.cur_sip=-1;
				a.cur_dip=-1;
				a.offset=0;
				a.option=option;
				a.valid=1;
				fdt.push_back(a);					//Update the file descriptor table and get the required inode and return the file index
				return fdt.size()-1;	
			}
		}

	}
	else if(option==WRONLY){				//if option is WRONLY
		for(i=0;i<5;i++){
			if(cwd.dp[i]==-1){				//If direct pointer has no block attached
				fileblock=fsystem.super.ffb;
				fsystem.super.ffb=*((int*)fsystem.blocks[fsystem.super.ffb-3].contents);
				memset(fsystem.blocks[fileblock-3].contents, 0, fsystem.super.block_size);	//get a free block for file first
				for(j=0;j<fsystem.super.ninodes;j++){
					if(fsystem.super.freeinodes[j]==0){		//Get a free inode for the file with DP[0] attached to free block
						inodeno=j;
						fileinode=&fsystem.ilist.inode_list[j];
						strcpy(fileinode->name,filename);
						fileinode->type=REGULAR;
						fileinode->size=0;
						fileinode->dp[0]=fileblock;
						fsystem.super.freeinodes[j]=1;		
						break;
					}
				}
				if(j==fsystem.super.ninodes)				//if no free inode is found
					return -1;
				dirfreeblock=fsystem.super.ffb;				//get a free block for directory
				fsystem.super.ffb=*((int*)fsystem.blocks[fsystem.super.ffb-3].contents);
				memset(fsystem.blocks[dirfreeblock-3].contents, 0, fsystem.super.block_size);

				directory* d=(directory*)fsystem.blocks[dirfreeblock-3].contents;
				for(j=0;j<fsystem.super.block_size/sizeof(directory);j++)
				{
					d[j].inodeno=-1;						//store inode number -1 for all the entries in directory
				}
				strcpy(d->fname,filename);
				d->inodeno=inodeno;							//store the above file in the directory block
				fsystem.ilist.inode_list[cur_dir].size++;
				fsystem.ilist.inode_list[cur_dir].dp[i]=dirfreeblock;		//update the DP of the directory inode
				FD a;
				a.file=&fsystem.ilist.inode_list[inodeno];		//Add the entry to file description table
				a.cur_dp=0;
				a.cur_sip=-1;
				a.cur_dip=-1;
				a.offset=0;
				a.option=option;
				a.valid=1;
				fdt.push_back(a);
				return fdt.size()-1;					//return the file description index
			}
			inodeno=searchDP_Directory_WR(filename,cwd.dp[i]);		//If the DP of directory is attached to some block
			if(inodeno!=-1){							//If I have got a free entry and inode for file
				blockno=fsystem.ilist.inode_list[inodeno].dp[0];
				memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);

				for(j=1;j<5;j++){						//Clear all the blocks of the file in Direct pointers
					blockno=fsystem.ilist.inode_list[inodeno].dp[j];
					if(blockno==-1) break;
					memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
					*((int*)fsystem.blocks[blockno-3].contents)=fsystem.super.ffb;
					fsystem.super.ffb=blockno;
					fsystem.ilist.inode_list[inodeno].dp[j]=-1;
				}
				blockno=fsystem.ilist.inode_list[inodeno].sip;	//Clear SIP
				if(blockno!=-1){
					clearSIP(blockno);

					memset(fsystem.blocks[blockno-3].contents,0,fsystem.super.block_size);
					*((int*)fsystem.blocks[blockno-3].contents)=fsystem.super.ffb;
					fsystem.super.ffb=blockno;
					fsystem.ilist.inode_list[inodeno].sip=-1;		//Free the SIP block
				}

				blockno=fsystem.ilist.inode_list[inodeno].dip;	//Clear DIP
				if(blockno!=-1){
					clearDIP(blockno);

					memset(fsystem.blocks[blockno-3].contents,0,fsystem.super.block_size);
					*((int*)fsystem.blocks[blockno-3].contents)=fsystem.super.ffb;
					fsystem.super.ffb=blockno;
					fsystem.ilist.inode_list[inodeno].dip=-1;		//Free the DIP block
				}
				fsystem.ilist.inode_list[inodeno].size=0;
				FD a;
				a.file=&fsystem.ilist.inode_list[inodeno];			//Add the entry in File description table
				a.cur_dp=0;
				a.cur_sip=-1;
				a.cur_dip=-1;
				a.offset=0;
				a.option=option;
				a.valid=1;
				fdt.push_back(a);
				return fdt.size()-1;		//Return the index
			}
		}

	}
	return -1;
}

int my_read(int fd, char *buf, int len){
	if(fd>=fdt.size() || fdt[fd].valid==0 || (fdt[fd].option!=RDONLY&&fdt[fd].option!=RDWR))
		return -1;
	int i,char_count=0,cur_block;
	FD *a = &fdt[fd];	//pointer to the FDT entry
	int *d,*s;
	int cur_sip;
	char *file;
	while(char_count < len){
		if(a->cur_dip==-1){
			if(a->cur_sip==-1){
				cur_block=a->file->dp[a->cur_dp];	//next block to be read is within blocks pointer to by direct pointer
			}
			else{
				//next block to be read is from singly indirect pointer
				if(a->file->sip==-1)	//there are no more blocks in the file
					return char_count;
				d=(int*)fsystem.blocks[a->file->sip-3].contents;
				cur_block=d[a->cur_dp];
			}
		} 

		else{
			//next block to be read is from doubly indirect pointer
			if(a->file->dip==-1)	//there are no more blocks in the file
				return char_count;
			s=(int*)fsystem.blocks[a->file->dip-3].contents;
			cur_sip=s[a->cur_sip];
			if(cur_sip==-1)
				return char_count;
			d=(int*)fsystem.blocks[cur_sip-3].contents;
			cur_block=d[a->cur_dp];
		}

		if(cur_block==-1)	//there are no more blocks in the file
			return char_count;

		file=fsystem.blocks[cur_block-3].contents;
		//read from the current block
		for(i=a->offset;i<fsystem.super.block_size && char_count<len;i++){
			buf[char_count++]=file[i];
			if(file[i]=='\0'){
				a->offset=i;
				return char_count;
			}
		}
		if(i!=fsystem.super.block_size){
			//if len number of characters have been read, return
			a->offset=i;
			return char_count;
		}

		//if end of block is reached and more characters are to be read,
		//update the offsets
		if(a->cur_dip==-1){
			if(a->cur_sip==-1){
				a->cur_dp++;
				if(a->cur_dp==5){
					a->cur_dp=0;
					a->cur_sip=0;
				}
			}
			else{
				a->cur_dp++;
				if(a->cur_dp==fsystem.super.block_size/sizeof(int)){
					a->cur_dp=0;
					a->cur_sip=0;
					a->cur_dip=0;
				}
			}
		}
		else{
			a->cur_dp++;
			if(a->cur_dp==fsystem.super.block_size/sizeof(int)){
				a->cur_dp=0;
				a->cur_sip++;
				if(a->cur_sip==fsystem.super.block_size/sizeof(int)){
					a->cur_dp=0;
					a->cur_sip=-1;
					a->cur_dip=-1;
					return char_count;
				}
			}
		}
		a->offset=0;
	}
	return char_count;
}

int my_write(int fd, const char *buf, int len){
	if(fd>=fdt.size() || fdt[fd].valid==0 || (fdt[fd].option!=WRONLY&&fdt[fd].option!=RDWR))
		return -1;
	if(len==0)
		return 0;
	int i,char_count=0,cur_block,blockno;
	FD *a = &fdt[fd];	//pointer to the FDT entry
	if(a->cur_dip==2)	//end of maximum blocks has been reached earlier
		return -1;
	int *d,*s;
	int cur_sip;
	char *file;

	while(char_count<len){
		if(a->cur_dip==-1){
			if(a->cur_sip==-1){
				//next block to be written to is within blocks pointer to by direct pointer
				cur_block=a->file->dp[a->cur_dp];
				if(cur_block==-1){
					//if no block is allocated to direct pointer, allocate new block
					blockno=fsystem.super.ffb;
					fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
					memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
					a->file->dp[a->cur_dp]=blockno;
					cur_block=blockno;
				}
			}
			else{
				//next block to be written to is from singly indirect pointer
				if(a->file->sip==-1){
					//allocate a block to store direct pointers
					blockno=fsystem.super.ffb;
					fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
					memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
					a->file->sip=blockno;

					//allocate a block to the 1st direct pointer in the singly indirect pointer
					blockno=fsystem.super.ffb;
					fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
					memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);

					*((int*)fsystem.blocks[a->file->sip-3].contents)=blockno;
					cur_block=blockno;
				}
				else{
					//singly indirect pointer already has a block
					d=(int*)fsystem.blocks[a->file->sip-3].contents;
					cur_block=d[a->cur_dp];
					if(cur_block==-1){
						//allocate a block to the next direct pointer in the singly indirect pointer
						blockno=fsystem.super.ffb;
						fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
						memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
						d[a->cur_dp]=blockno;
						cur_block=blockno;
					}
				}
			}
		} 

		else{
			//next block to be written to is from singly indirect pointer
			if(a->file->dip==-1){
				//allocate a block to store singly indirect pointers
				blockno=fsystem.super.ffb;
				fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
				memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
				a->file->dip=blockno;
				s=(int*)fsystem.blocks[blockno-3].contents;
				
				//allocate a block to store direct pointers
				blockno=fsystem.super.ffb;
				fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
				memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
				*s=blockno;

				d=(int*)fsystem.blocks[blockno-3].contents;
				
				//allocate a block to the 1st direct pointer in the singly indirect pointer
				blockno=fsystem.super.ffb;
				fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
				memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
				*d=blockno;
				cur_block=blockno;

			}
			else{
				//doubly indirect pointer already has a block
				s=(int*)fsystem.blocks[a->file->dip-3].contents;
				cur_sip=s[a->cur_sip];
				if(cur_sip==-1){
					//allocate a block to the next singly indirect pointer in the doubly indirect pointer
					blockno=fsystem.super.ffb;
					fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
					memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
					s[a->cur_sip]=blockno;

					d=(int*)fsystem.blocks[blockno-3].contents;
					
					//allocate a block to the 1st direct pointer in the singly indirect pointer
					blockno=fsystem.super.ffb;
					fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
					memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
					*d=blockno;
					cur_block=blockno;
				}
				else{
					d=(int*)fsystem.blocks[cur_sip-3].contents;
					cur_block=d[a->cur_dp];
					if(cur_block==-1){
						//allocate a block to the next direct pointer in the singly indirect pointer
						blockno=fsystem.super.ffb;
						fsystem.super.ffb=*((int*)fsystem.blocks[blockno-3].contents);
						memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
						d[a->cur_dp]=blockno;
						cur_block=blockno;
					}
				}
				
			}
		}

		file=fsystem.blocks[cur_block-3].contents;
		//write into the current block
		for(i=a->offset;i<fsystem.super.block_size && char_count<len;i++){
			file[i]=buf[char_count++];
			a->file->size++;
			if(file[i]=='\0'){
				a->offset=i;
				return char_count;
			}
		}

		if(i!=fsystem.super.block_size){
			//if all characters have been written, return
			a->offset=i;
			return char_count;
		}

		//if end of block is reached and more characters are to be written,
		//update the offsets
		if(a->cur_dip==-1){
			if(a->cur_sip==-1){
				a->cur_dp++;
				if(a->cur_dp==5){
					a->cur_dp=0;
					a->cur_sip=0;
				}
			}
			else{
				a->cur_dp++;
				if(a->cur_dp==fsystem.super.block_size/sizeof(int)){
					a->cur_dp=0;
					a->cur_sip=0;
					a->cur_dip=0;
				}
			}
		}
		else{
			a->cur_dp++;
			if(a->cur_dp==fsystem.super.block_size/sizeof(int)){
				a->cur_dp=0;
				a->cur_sip++;
				if(a->cur_sip==fsystem.super.block_size/sizeof(int)){
					a->cur_dp=-1;
					a->cur_sip=2;
					a->cur_dip=2;
					return char_count;
				}
			}
		}
		a->offset=0;

	}

	return char_count;
}

int my_mkdir(const char * dirname){				//Make directory
	int i,j,k,inodeno,dirblock,blockno;
	for(i=0;dirname[i]!='\0';i++)				//Assumption that A-Za-z_ are only allowed
	{
		if(!((dirname[i]>='A'&&dirname[i]<='Z')||(dirname[i]>='a'&&dirname[i]<='z')||dirname[i]=='_'))
			return -1;
	}

	directory *d,*newd;						
	Inode cwd=fsystem.ilist.inode_list[cur_dir];	//Get current working directory
	Inode* dirinode;
	if(cwd.type!=DIRECTORY) return -1;
	for(i=0;i<5;i++){
		if(cwd.dp[i]==-1){						//If after searching I reach a point where first i-1 directory blocks are full and ith is not allocated
			dirblock=fsystem.super.ffb;
			fsystem.super.ffb=*((int*)fsystem.blocks[fsystem.super.ffb-3].contents);
			memset(fsystem.blocks[dirblock-3].contents, 0, fsystem.super.block_size);
			fsystem.ilist.inode_list[cur_dir].dp[i]=dirblock;		//Get a free block for directory's ith pointer

			d=(directory*)fsystem.blocks[dirblock-3].contents;		
			for(j=0;j<fsystem.super.block_size/sizeof(directory);j++){
				d[j].inodeno=-1;				//Store -1 in all inode numebrs
			}

			dirblock=fsystem.super.ffb;			//Get a free block for new directory
			fsystem.super.ffb=*((int*)fsystem.blocks[fsystem.super.ffb-3].contents);
			memset(fsystem.blocks[dirblock-3].contents, 0, fsystem.super.block_size);

			
			for(k=0;k<fsystem.super.ninodes;k++){		//Get a free inode for new directory
				if(fsystem.super.freeinodes[k]==0){
					inodeno=k;
					dirinode=&fsystem.ilist.inode_list[k];
					strcpy(dirinode->name,dirname);
					dirinode->type=DIRECTORY;
					dirinode->size=2;
					dirinode->dp[0]=dirblock;
					fsystem.super.freeinodes[k]=1;
					break;
				}
			}
			if(k==fsystem.super.ninodes)				//If no free inode is found
				return -1;
			strcpy(d[0].fname,dirname);					//Store the new directory name in cwd
			d[0].inodeno=inodeno;
			newd=(directory*)fsystem.blocks[dirblock-3].contents;	//Store the entries for . and .. in new directory
			strcpy(newd[0].fname,".");
			newd[0].inodeno=inodeno;
			strcpy(newd[1].fname,"..");
			newd[1].inodeno=cur_dir;
			fsystem.ilist.inode_list[cur_dir].size+=2;
			for(k=2;k<fsystem.super.block_size/sizeof(directory);k++){
				newd[k].inodeno=-1;			
			}		
			return 0;
		}

		d=(directory*)fsystem.blocks[cwd.dp[i]-3].contents;			//Otherwise search the directory
		for(j=0;j<fsystem.super.block_size/sizeof(directory);j++){	
			if(strcmp(d[j].fname,dirname)==0)		//If directory already exists
				return -1;	
			if(d[j].inodeno==-1){					//If I find first free entry
				dirblock=fsystem.super.ffb;
				fsystem.super.ffb=*((int*)fsystem.blocks[fsystem.super.ffb-3].contents);
				memset(fsystem.blocks[dirblock-3].contents, 0, fsystem.super.block_size);
				//Find a free block

				for(k=0;k<fsystem.super.ninodes;k++){		//Find a free inode
					if(fsystem.super.freeinodes[k]==0){
						inodeno=k;
						dirinode=&fsystem.ilist.inode_list[k];
						dirinode->type=DIRECTORY;
						strcpy(dirinode->name,dirname);
						dirinode->size=2;
						dirinode->dp[0]=dirblock;
						fsystem.super.freeinodes[k]=1;
						break;
					}
				}
				if(k==fsystem.super.ninodes)			//If no inodes remain
					return -1;
				strcpy(d[j].fname,dirname);
				d[j].inodeno=inodeno;
				newd=(directory*)fsystem.blocks[dirblock-3].contents;	//Open the first block of new directory and store . and ..
				strcpy(newd[0].fname,".");
				newd[0].inodeno=inodeno;
				strcpy(newd[1].fname,"..");
				newd[1].inodeno=cur_dir;
				fsystem.ilist.inode_list[cur_dir].size+=2;
				for(k=2;k<fsystem.super.block_size/sizeof(directory);k++){
					newd[k].inodeno=-1;			
				}
				return 0;
			}
		}
	}

}

int my_chdir(const char* dirname){
	int i,j,k,inodeno,blockno;
	for(i=0;dirname[i]!='\0';i++)
	{
		if(dirname[i]=='/')	
			return -1;
	}
	directory *d;
	Inode cwd=fsystem.ilist.inode_list[cur_dir];	//Get the current working directory
	Inode* dirinode;
	if(cwd.type!=DIRECTORY){ return -1; }
	for(i=0;i<5;i++){
		blockno=cwd.dp[i];
		if(blockno==-1){	//If we reach the end of directory
			return -1;
		}
		d=(directory*)fsystem.blocks[blockno-3].contents;
		for(j=0;j<fsystem.super.block_size/sizeof(directory);j++){
			if(d[j].inodeno==-1){	//If we reach the end of directory
				return -1;
			}
			if(d[j].inodeno==-2)
				continue;
			if(strcmp(d[j].fname,dirname)==0){
				//If directory is found, change the current directory
				inodeno=d[j].inodeno;
				if(fsystem.ilist.inode_list[inodeno].type==DIRECTORY){
					cur_dir=inodeno;
					cout<<"You are now in Directory "<<fsystem.ilist.inode_list[inodeno].name<<endl;
					return 0;
				}
			}
		}
	}
	return -1;
}

int clearDirectory(int inodeno){			//Clear directory (before removing the directory)

	Inode* dirinode=&fsystem.ilist.inode_list[inodeno];
	int i,j,k,l,blockno;
	directory* d;
	for(i=0;i<5;i++){
		if(dirinode->dp[i]==-1)				//When i find the first empty pointer implies my directory has been cleared
			return 0;
		d=(directory*)fsystem.blocks[dirinode->dp[i]-3].contents;	//go to contents of dp[i] block
		for(j=0;j<fsystem.super.block_size/sizeof(directory);j++){
			if(d[j].inodeno==-1)			//Once I get the first empty entry implies directory cleared
				return 0;
			if(strcmp(d[j].fname,".")==0||strcmp(d[j].fname,"..")==0)
				continue;
			if(fsystem.ilist.inode_list[d[j].inodeno].type==DIRECTORY){		//If the type of element is directory, recursively call the function
				clearDirectory(d[j].inodeno);
			}
			else if(fsystem.ilist.inode_list[d[j].inodeno].type==REGULAR){	//For a regular file
				for(k=0;k<5;k++){
					blockno=fsystem.ilist.inode_list[d[j].inodeno].dp[k];	//Clear the DPs iteratively
					if(blockno==-1) break;
					memset(fsystem.blocks[blockno-3].contents, 0, fsystem.super.block_size);
					*((int*)fsystem.blocks[blockno-3].contents)=fsystem.super.ffb;
					fsystem.super.ffb=blockno;								//Free the blocks
					fsystem.ilist.inode_list[d[j].inodeno].dp[k]=-1;
				}
				blockno=fsystem.ilist.inode_list[d[j].inodeno].sip;		//Clear the SIP
				if(blockno!=-1){
					clearSIP(blockno);

					memset(fsystem.blocks[blockno-3].contents,0,fsystem.super.block_size);
					*((int*)fsystem.blocks[blockno-3].contents)=fsystem.super.ffb;
					fsystem.super.ffb=blockno;
					fsystem.ilist.inode_list[d[j].inodeno].sip=-1;		//Free the SIP block
				}

				blockno=fsystem.ilist.inode_list[d[j].inodeno].dip;
				if(blockno!=-1){
					clearDIP(blockno);									//Clear the DIPs

					memset(fsystem.blocks[blockno-3].contents,0,fsystem.super.block_size);
					*((int*)fsystem.blocks[blockno-3].contents)=fsystem.super.ffb;
					fsystem.super.ffb=blockno;
					fsystem.ilist.inode_list[d[j].inodeno].dip=-1;		//Free the DIP block
				}

				fsystem.ilist.inode_list[d[j].inodeno].type=-1;			//Clear the inode for the file
				for(l=0;l<5;l++) fsystem.ilist.inode_list[d[j].inodeno].dp[l]=-1;
				fsystem.ilist.inode_list[d[j].inodeno].sip=-1;
				fsystem.ilist.inode_list[d[j].inodeno].dip=-1;
				fsystem.super.freeinodes[d[j].inodeno]=0;
				strcpy(fsystem.ilist.inode_list[d[j].inodeno].name,"");

			}
		}
		blockno=dirinode->dp[i];			//Clear the ith block in DP of directory
		memset(fsystem.blocks[blockno-3].contents,0,fsystem.super.block_size);
		*((int*)fsystem.blocks[blockno-3].contents)=fsystem.super.ffb;
		fsystem.super.ffb=blockno;
	}
	fsystem.ilist.inode_list[inodeno].type=-1;	//Clear the inode for the directory
	for(j=0;j<5;j++) fsystem.ilist.inode_list[inodeno].dp[j]=-1;
	fsystem.ilist.inode_list[inodeno].sip=-1;
	fsystem.ilist.inode_list[inodeno].dip=-1;
	fsystem.super.freeinodes[inodeno]=0;
	strcpy(fsystem.ilist.inode_list[inodeno].name,"");
	return 0;
}

int my_rmdir(const char * dirname){				//Remove a directory
	int i,j,k,inodeno,blockno;
	if(strcmp(dirname,".")==0||(strcmp(dirname,"..")==0))	//Can't remove . or ..
		return -1;
	for(i=0;dirname[i]!='\0';i++)				//Can't remove complex paths: Assumption
	{
		if(dirname[i]=='/')						
			return -1;
	}
	directory *d;
	Inode cwd=fsystem.ilist.inode_list[cur_dir];	//Get the current working directory
	Inode* dirinode;
	if(cwd.type!=DIRECTORY) return -1;		
	for(i=0;i<5;i++){
		blockno=cwd.dp[i];
		if(blockno==-1)			//If we reach the end of directory
			return -1;
		d=(directory*)fsystem.blocks[blockno-3].contents;
		for(j=0;j<fsystem.super.block_size/sizeof(directory);j++){
			if(d[j].inodeno==-1)	//If we reach the end of directory
				return -1;
			if(strcmp(d[j].fname,dirname)==0){		//If directory to be removed is found
				inodeno=d[j].inodeno;		
				if(fsystem.ilist.inode_list[inodeno].type==DIRECTORY){	//if the type is directory
					clearDirectory(inodeno);		//clear it
					d[j].inodeno=-2;
					return 0;
				}
			}
		}
	}
	return -1;
}

int my_close(int fd){			//Close a file only if it is valid and within the range of FDT
	if(fd>=fdt.size() || fdt[fd].valid==0)
		return -1;
	fdt[fd].valid=0;
	return 0;
}

int my_cat(const char *filename){		//cat/ print a file on terminal
	int i,n;
	int fd=my_open(filename,RDONLY);
	if(fd==-1)	//file does not exist
		return -1;
	char buf[20];
	//read upto end of file and print
	while(1){
		n=my_read(fd,buf,10);			//read
		for(i=0;i<n;i++){
			cout<<buf[i];				//print
		}
		if(buf[n-1]=='\0')
			break;
	}
	cout<<endl;
	my_close(fd);					//close the file
	return 0;

}

int my_copy(const char *filename1, const char * filename2, int file1_type){
	int from_fd,to_fd,n,m;
	char buf;
	if(file1_type==LINUX_FILE){
		//if 1st file is of Linux file system
		from_fd=open(filename1,O_RDONLY);		//Open one file in LINUX, other in ALT2. 
		to_fd=my_open(filename2,WRONLY);
		if(from_fd==-1 || to_fd==-1)
			return -1;
		while(1){
			n=read(from_fd,&buf,1);				//Read from Linux, write into ALT2 file
			if(buf=='\0')
				break;
			m=my_write(to_fd,&buf,n);
			if(m==-1) return -1;
		}	
		close(from_fd);
		my_close(to_fd);
	}
	else{
		from_fd=my_open(filename1,RDONLY);		//Open one file in LINUX, other in ALT2. 
		to_fd=open(filename2,O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if(from_fd==-1 || to_fd==-1)
			return -1;
		while(1){
			n=my_read(from_fd,&buf,1);			//Read from ALT2, write into Linux file
			if(buf=='\0')
				break;
			m=write(to_fd,&buf,1);				
			if(m==-1) return -1;
		}
		my_close(from_fd);
		close(to_fd);
	}
	return 0;
}

int initialize(int totalsize, int blocksize){

	int nblocks=totalsize/blocksize; 				//Get number of blocks and number of inodes
	int ninodes=blocksize*2/sizeof(Inode);
	if(sizeof(Block0)+ninodes*sizeof(int)>blocksize){	//Superblock should fit in block 0
			cout<<"Invalid Input: Problem with Superblock Allocation \n";
			return -1;
	}
	cur_dir=0;						//Current directory inode is 0 and is the root Directory
	fsystem.super.block_size=blocksize;		//Superblock
	fsystem.super.tot_size=totalsize;
	fsystem.super.nblocks=nblocks;
	fsystem.super.ninodes=ninodes;
	strcpy(fsystem.super.name,"Alternative2");

	fsystem.super.freeinodes = new int[ninodes];		//List of inodes
	int i,j;
	for(i=1;i<ninodes;i++) fsystem.super.freeinodes[i]=0;	//All are free initially
	fsystem.super.freeinodes[0]=1;	//Inode 0 is full
	fsystem.super.ffb=3;			//Block 3 is First free block

	fsystem.ilist.inode_list=new Inode[ninodes];
	for(i=0;i<ninodes;i++)			//Initialise all inodes
	{
		fsystem.ilist.inode_list[i].type=-1;
		for(j=0;j<5;j++) fsystem.ilist.inode_list[i].dp[j]=-1;
		fsystem.ilist.inode_list[i].sip=-1;
		fsystem.ilist.inode_list[i].dip=-1;
	}

	fsystem.blocks=new Other[nblocks-3];	//Initialise all blocks with block i storing the index of (i+1)th block
	for(i=0;i<nblocks-3;i++)
	{
		fsystem.blocks[i].contents=new char[blocksize];
		memset(fsystem.blocks[i].contents,0,blocksize);
	}
	int *a;
	for(i=3;i<nblocks-1;i++)				
	{
		a=(int*)fsystem.blocks[i-3].contents;
		*a=i+1;
	}
	a=(int*)fsystem.blocks[nblocks-4].contents;
	*a=-1;									//Last block stores -1

	//Create root directory
	Inode *dirinode=&fsystem.ilist.inode_list[0];
	dirinode->type=DIRECTORY;
	dirinode->size=0;
	strcpy(dirinode->name,"root");
	int dirfreeblock=fsystem.super.ffb;
	fsystem.super.ffb=*((int*)fsystem.blocks[fsystem.super.ffb-3].contents);
	directory* d=(directory*)fsystem.blocks[dirfreeblock-3].contents;

	memset(fsystem.blocks[dirfreeblock-3].contents, 0, fsystem.super.block_size);

	dirinode->dp[0]=dirfreeblock;
	strcpy(d[0].fname,".");			//Root directory only has '.', no '..'
	d[0].inodeno=0;

	for(i=1;i<fsystem.super.block_size/sizeof(directory);i++){
		d[i].inodeno=-1;			
	}
	return 0;
}
