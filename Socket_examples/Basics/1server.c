#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write


////en este codigo, el server recibe msj enviados por el cliente.
//
///// gcc -o server server.c

int main(int argc , char *argv[])
{
	int descriptor;
	//Create socket
	descriptor= socket(AF_INET , SOCK_STREAM , 0);//zero means TCP protocol
	if (descriptor== -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");
	
	
	struct sockaddr_in server;
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY if you want to bind to your local IP. Here you specify the ip address
					     //Also can use inet_addr("ip numbers and dots") or inet_ntoa("ip numbers and dots")
	server.sin_port = htons( 2010  );//server port

	//bind(descriptor,struct, size struct) this function associates a local address and a port to the socket.
	if(bind(descriptor,(struct sockaddr*)&server,sizeof(server))<0){
		perror("Error binding");
		return 1;
	}
	puts("Bind Done!\n");

	//listen() and connect()
	//if you want to wait for incoming connections. We have incoming connections in a queue until we accept them
	//and there is a limit on how many can queue up
	//
	listen(descriptor,4); ///specify the maximum number of connections COVID-19
	puts("Waiting for incoming connections\n");

	////accept() returns a single file descriptor for every new connection, while de fd of the server continues listening
	//accept() blocks until an incoming connection is made to the listening socket's port number.
	struct sockaddr_in client;
	
	int c=sizeof(struct sockaddr_in);
	
	int clientSock=accept(descriptor,(struct sockaddr *)&client,(socklen_t*)&c);
	
	if(clientSock<0){
		perror("Connection failed!\n");
		return 1;
	}
	printf("Your ticket number is: %d \n",clientSock);
	puts("Connection accepted\n");

	///recv()   función para recibir msj. It returns a positive value when sth is received.
	//recv() returns the number of bytes read into the buffer
	///recv() blocks until sth is received ot the client closes the connection
	///recv() returns 0 when the client closes the connection
	//parametros: descriptor, buffer, lenght of the buffer, 0
	
	int read_size; //recv() returns the number of bytes read into the buffer
	int len=200;
	char buffer[len];
	
	
	////read msj
	while((read_size=recv(clientSock,buffer,len,0))>0){
		puts(buffer);
		memset(buffer,0,sizeof(buffer)); ///limpiamos buffer
	} 
	///when the client loses connection, server receives a buffer 
	if(read_size==0){
		printf("\nClient desconnected\n");
		fflush(stdout);   ////aún no sé que hace
	}
	else if(read_size<0){
		perror("Recv failed");
	}

	return 0;
}
