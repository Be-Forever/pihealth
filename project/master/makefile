objects = new_sever.o common.o 
CFLAGS = -lpthread -lz -lmysqlclient

.PHONY:all
	all:server

server : $(objects)
	gcc $(objects) -o server $(CFLAGS) 

new_sever.o : new_sever.c 
	gcc -c new_sever.c $(CFLAGS)

common.o : ./common/common.c 
	gcc -c ./common/common.c $(CFLAGS)

headers :
	gcc -M new_sever.c  > headers

include headers



.PHONY : clean
clean:
	rm server *.o headers
