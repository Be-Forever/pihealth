objects = sec_client.o common.o
CFLAGS = -lpthread -lz

.PHONY:all
	all:client

client : $(objects)
		gcc $(objects) -o client $(CFLAGS)

sec_client.o : sec_client.c 
		gcc -c sec_client.c $(CFLAGS)

common.o : ./common/common.c 
		gcc -c ./common/common.c $(CFLAGS)

headers :
		gcc -M sec_client.c  > headers

include headers


.PHONY : clean
clean :
	rm client *.o headers
