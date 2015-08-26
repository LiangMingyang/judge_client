all: process_monitor

process_monitor: main.o process_block.o limit_result.o
	gcc -o process_monitor main.o process_block.o limit_result.o

main.o: main.c process_block.h limit_result.h
	gcc -c main.c

limit_result.o: limit_result.c limit_result.h
	gcc -c limit_result.c

process_block.o: process_block.c process_block.h
	gcc -c process_block.c

clean:
	rm *.o process_monitor process_watcher
