a.out: file2.o libalt2.a
	g++ file2.o -L. -lalt2

file2.o:	file2.cpp alt2.h
	g++ -c file2.cpp

libalt2.a:	alt2.o
		ar -rcs libalt2.a alt2.o

alt2.o:	alt2.cpp alt2.h
	g++ -c alt2.cpp 

clean:
	rm file2.o libalt2.a alt2.o a.out
