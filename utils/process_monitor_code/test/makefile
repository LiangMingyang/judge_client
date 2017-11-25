

all: t2 t3 t4


t2: t2.o
	cc -o t2 t2.o
t2.o: t2.c
	cc -c t2.c

t3: t3.o
	cc -o t3 t3.o
t3.o: t3.c
	cc -c t3.c


t4: t4.o
	cc -o t4 t4.o
t4.o: t4.c
	cc -c t4.c

clean:
	rm *.o t2 t3 t4
