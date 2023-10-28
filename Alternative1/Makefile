a.out: file.o libalt1.a
	g++ file.o -L. -lalt1

file.o:	file.cpp alt1.h
	g++ -c file.cpp

libalt1.a:	alt1.o
		ar -rcs libalt1.a alt1.o

alt1.o:	alt1.cpp alt1.h
	g++ -c alt1.cpp 

clean:
	rm file.o libalt1.a alt1.o a.out
