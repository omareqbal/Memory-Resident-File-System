#include "alt1.h"

int main(int argc,char *argv[])
{
	if(argc<3)
		{
		cout<<"Two Arguments Needed\n";
		return 0;
		}
	
	int totalsize=atoi(argv[1]);
	int blocksize=atoi(argv[2]);		//Give initial values
	
	char content[20],buf[20];

	int p=initialize(totalsize,blocksize);	//Initialise data
	if(p==-1) return 0;

	int fd=my_open("a.txt",WRONLY);
	strcpy(content,"Hey, how are you?");	//write this string in a.txt
	my_write(fd,content,strlen(content)+1);
	my_close(fd);

	fd=my_open("a.txt",RDONLY);		//open,read and print
	my_read(fd,buf,20);
	my_close(fd);
	cout<<buf<<endl;

	my_cat("a.txt");			//use my_cat to print

	int n=my_copy("b.txt","b.txt",LINUX_FILE);		//copy from Linux file system to alt1
	if(n==-1) 
		{
			cout<<"Copy failed\n"; exit(0);
		} 
	my_cat("b.txt");

	n=my_copy("b.txt","c.txt",ALT1_FILE);			//copy from alt1 file system to linux file system
	if(n==-1) 
		{
			cout<<"Copy failed\n"; exit(0);
		}
	
	
	fd=my_open("a.txt",RDWR);			//depiction of read-write mode
	my_read(fd,buf,4);
	my_write(fd,buf,4);
	my_close(fd);
	
	my_cat("a.txt");

	return 0;
}
