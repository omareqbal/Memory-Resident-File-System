#include "alt2.h"

int main(int argc,char* argv[]){
	if(argc<3){
		cout<<"Invalid arguments"<<endl;
		exit(1);
	}
	if(initialize(atoi(argv[1]),atoi(argv[2]))==-1){
		exit(1);
	}
	int fd=my_open("a.txt",WRONLY);
	char buf[300]="HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world HEllo world ";
	int c=my_write(fd,buf,300);
	my_close(fd);

	char buf2[300];
	fd=my_open("a.txt",RDONLY);
	c=my_read(fd,buf2,300);
	cout<<buf2<<" "<<c<<endl;
	my_close(fd);

	c=my_mkdir("Omar");
	cout<<"{{"<<c<<endl;
	c=my_chdir("Omar");
	cout<<"[["<<c<<endl;
	if(c==-1) cout<<"ERRORRRRR\n";
	c=my_chdir("Lovish");
	if(c==-1) cout<<"Error\n";

	fd=my_open("a.txt",RDONLY);
	cout<<fd<<endl;

	fd=my_open("b.txt",WRONLY);
	my_write(fd,"hello",6);
	my_close(fd);

	char buf3[10];
	fd=my_open("b.txt",RDONLY);
	my_read(fd,buf3,10);

	cout<<buf3<<endl;
	my_chdir("..");
	fd=my_open("b.txt",RDONLY);
	cout<<fd<<endl;

	fd=my_open("a.txt",RDONLY);
	cout<<fd<<endl;
	
	cout<<my_rmdir("Omar")<<endl;
	cout<<my_chdir("Omar")<<endl;
	my_cat("a.txt");
	my_copy("a.txt","a.txt",ALT2_FILE);
}