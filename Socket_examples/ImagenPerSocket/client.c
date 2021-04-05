#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){

    int sfd =0, b;
    char rbuff[1024];
    char sendbuffer[100];
/////////////////////////////////////////////////////////////////////////////////////////
/////creacion y enlace de sockets
    struct sockaddr_in serv_addr;

    memset(rbuff, '0', sizeof(rbuff));
    sfd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    b=connect(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (b==-1) {
        perror("Connect");
        return 1;
    }
///////////////////////////////////////////////////////////////////////////////////////////
    FILE *fp = fopen("land.bmp", "rb");   /////se abre el archivo que deseamos enviar.
    if(fp == NULL){
        perror("File");
        return 2;
    }
    
    int n;  
	int total=0;
	long int position;
	///////////////fread() The total number of elements successfully read are returned.
	///////////////   n saves the number returned
	//// fread(BUFFER DONDE QUEREMOS GUARDAR,
	/////		TAMAÑO EN BYTES DE CADA ELEMENTO SINGULAR POR LEER,
	/////		CANTIDAD DE ELEMENTOS POR LEER,
	////			PUNTERO DEL ARCHIVO LEÍDO)
	

    while( (n = fread(sendbuffer, 1, sizeof(sendbuffer), fp)) > 0 ){
    	printf("Amount of bytes: %d \n",n);
    	total+=n;
        send(sfd, sendbuffer, n, 0);   /////se envía el buffer leído por el socket
        position=ftell(fp);  ////esta función nos dice en donde está ubicado el pointer en el archivo
        printf("Actual position:%ld \n",position);
        
    }
	printf("la cantidad total de bytes leido es: %d \n",total);
	long size=0;
	
	fseek(fp,0,SEEK_END);
	size=ftell(fp);
	printf("Size of this image:%ld\n",size);
	char str[256];
	sprintf(str,"%ld",size);
	printf("size en string %s\n\n",str);
	fseek(fp,0,SEEK_SET);
	size=ftell(fp);
	printf("position:%ld\n\n",size);
	
    fclose(fp);
    return 0;

}
