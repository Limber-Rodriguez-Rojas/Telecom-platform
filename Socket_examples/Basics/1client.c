#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

/////en este codigo, el cliente envia msj hacia el server.
//es un codigo bastante basico para aprender lo esencial de sockets

// gcc -o client client.c
int main(int argc, char *argv[]){
	

	int descriptor=socket(AF_INET,SOCK_STREAM,0);  ////crea un socket (dominio,tipo,protocol), returns a socket descriptor
	if(descriptor==-1){
		printf("Error creating the socket");
	}
	puts ("Socket created\n");


	struct sockaddr_in server; ///structura donde se guardará toda la info del servidor(CONTAINER)
	///The call to memset() ensures that any parts of the structure thar we don't explicitly ser contain zero.
	
	server.sin_addr.s_addr=inet_addr("18.191.92.207");  ////here you put the server's IP
	//Also can use inet_addr("ip numbers and dots") or inet_ntoa("ip numbers and dots")
	server.sin_family=AF_INET;
	server.sin_port=htons(2010); // here you specify the port number

	if(connect(descriptor, (struct sockaddr *)&server, sizeof(server))  <0){   //the connect() function stablishes a connection between the given socket and the one identified by the IP addres and port in sockaddr_in.
		perror("connect faliled. Error");
		return 1;
	}

	puts("Connected\n");
	
	///recv()   función para recibir msj. It returns a positive value when sth is received. It returns a number of bytes read into the buffer
	///recv() blocks until sth is received ot the client closes the connection
	/// recv() returns 0 when the client closes the connection
	
	
	//write(descriptor, buffer, tamaño buffer)
	char buffer[100]="Hi server"; 
	int check;
	
	while(1){
		printf("Enter msj: ");
		scanf("%s",buffer);
		send(descriptor,buffer,strlen(buffer),0); //send(descriptor socket,msj,tamaño de msj,0 por default)
		printf("Msj sent\n\n");
	}
	
	
///	check=write(descriptor,buffer,strlen(buffer));
	
	close(descriptor);
	return 0;










}
