# Make file to compile server and client

all:
	gcc pc_Asy.c -o pc
	gcc rasp_Asy.c -o rasp
	gcc client.c -o phone
	gcc usuario.c -o user
	gcc awserver.c -o aws $(mariadb_config --libs --cflags)	
	
clean:
	rm rasp
	rm pc
	rm phone
	rm user
	rm aws

