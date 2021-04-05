#include <stdio.h> //printf
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h> 
#include <stdbool.h>
#include <pthread.h>

#include </home/an/proyectotallercomu/completo/cliente.h>
//directives are above (e.g. #include ...)

///gcc -o c client.c -lpthread

#define COMPLETE 0
#define BUF_SIZE 256
#define maxConectados 5


char client_name[256];  ///where the name is stored
char conectados[maxConectados];

char message[256] , server_reply[256]; //message is the buffer where the keyboard input is saved. //server_reply is the buffer where the incoming msj is saved.
char msjFinal[256];//msjFinal es el que se envía.
int maxfd;//to use on select()
int sock;// file descriptor of the socket. 
fd_set all_set, r_set; //file descriptors to use on select()
struct timeval tv;

////this parameters are used to send the msj
int n,b,tot; 
int total=0;
long int position;
char sendbuffer[1];
char buff[1];//cantidad de bytes enviado por turno, si se cambia acá cambiar en la funcion donde recibe
long imageSize;

////functions to use with threads
void *menu();
void *sockets();

int main(int argc , char **argv){

	get_client_name(argc,argv,client_name); ///this function gets the client name.    ./client NAME
	printf("Client: '%s'\n",client_name);
	
	//Create socket
	struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    maxfd = sock + 1;
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    server.sin_addr.s_addr = inet_addr("172.24.85.141");///change this IP for the IP of your terminal socket
    server.sin_family = AF_INET;
    server.sin_port = htons( 9034 );   /////the port of the socket. Has to be the same than the server port
	tv.tv_sec = 2; 
	tv.tv_usec = 0;
    
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){  //Connect to remote server
        perror("connect failed. Error");
        return 1;
    }
	puts("Connected\n");    
	send(sock,client_name,strlen(client_name),0); ///send the name of this client to the server. The server has a register of every connected client
	
	//set up variables for select()
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set); FD_SET(sock, &all_set);
	r_set = all_set;
	
	//set the initial position of after
	after = message;
/////////////Thread who manages sockets(receive msj from server)/////////////
    pthread_t hilo;
	int ph=pthread_create(&hilo,NULL, sockets,NULL);
	if(ph!=0){
		printf("Error al crear el hilo\n");
		return -1;
	}
///////////////////Thread that manages the menu//////////////////
	pthread_t menuh;
	int mph=pthread_create(&hilo,NULL, menu,NULL);
	if(mph!=0){
		printf("Error al crear el hilo\n");
		return -1;
	}
	
	while(1){}
	return 0;
}



/////////////////////////////////////////  Function to manage the menu with a thread //////////////
void *menu(){
	while(1){
		printf("\n\n-----------SELECT YOUR OPTION: -------------\n 1. Recargar \n 2. Consultar cliente \n 3. Chat \n 4. Exit\n");
		int option;
		char destinatario; //// to save the  recipient client of the msj
		bool chat=false;  ////true to get in the chat, false to get out
		scanf("%d",&option); //to get the option
		printf("\nOpcion selecionada %d\n\n",option);
		if(option==1){}
		if(option==2){}
		
		
		
		
	////////////////////////////////////////////////////	CHAT
		if(option==3){
			printf("Bienvenido al chat\nPara salir del chat presione X\n\n");
			
			for(int y=0;y<maxConectados;y++){
				if(conectados[y]!='\0'){
				printf("Usuarios conectados:%c\n",conectados[y]);			
				}
			}		
			printf("Seleccione destinatario ");
			scanf(" %c",&destinatario);
			if(destinatario=='X'){ //if 'X' is selected it gets out of the chat
				chat =false;
				printf("Saliendo del chat\n\n");
			}
			printf("\nDestinatario seleccionado es:%c\n",destinatario);
			chat=true;
			
			while(chat){

				r_set = all_set;
				select(maxfd, &r_set, NULL, NULL, &tv);//check to see if we can read from STDIN or sock

				if(FD_ISSET(STDIN_FILENO, &r_set)){///this function is merely to write a msj and send it! or to send the image. The way to receive msj is on the other thread!
					if(buffer_message(message) == COMPLETE){////if the keyboard input is done...do this
						memset(msjFinal,'\0',sizeof(msjFinal));////clean the buffer that is gonna be sent
						
						if(message[0]!='X'){// 'X' was to get out of the chat, if it is not 'X' we are going to send a msj
							if(message[0]!='?'){ ///we are not sending an image. We are sending a normal msj
								msjFinal[0]=destinatario; ///  insert the recipient client in the begining of the Final msj
								strcat(msjFinal,message); ///  inserte the rest of the msj
								strcat(msjFinal," FROM:"); 
								strcat(msjFinal,client_name);  ///the msj is sended FROM client_name
								if(send(sock, msjFinal, sizeof(msjFinal), 0) < 0){//////envia el msj
									puts("Send failed");
									//return 1;
								}	
							}
							//if the msj is not a normal text, we are sending an Image
							else{       ////sending an image
								msjFinal[0]=destinatario; ///  insert the recipient client in the begining of the Final msj
								strcat(msjFinal,message); ///  inserte the rest of the msj
								strcat(msjFinal," FROM:");
								strcat(msjFinal,client_name); ///the msj is sended FROM client_name
								printf("Enviado imagen\n");
								send(sock, msjFinal, sizeof(msjFinal), 0); ////this msj is sending the symbol '?'
								
								FILE *fp = fopen("land.bmp", "rb");   ///// open the image that we are gonna send!
								if(fp == NULL){
									perror("File");
									//return 2;
								}
								///we need to send the size of the file 
								long size=0;
								fseek(fp,0,SEEK_END);///esto es para informar el tamaño total del file, puntero se ubica al final
								size=ftell(fp); //se guarda posicion del puntero
								memset(msjFinal,'\0',sizeof(msjFinal)); //se limpia la memoria del msj
								sprintf(msjFinal,"%ld",size);///de long a str, se guarda en el buffer a enviar
								send(sock, msjFinal, sizeof(msjFinal) , 0);// se envia el tamaño
								fseek(fp,0,SEEK_SET);//se vuelve a colocar el puntero al inicio del file
								total=0; // to count the amount of bytes sent
								
								while( (n = fread(sendbuffer, 1, sizeof(sendbuffer), fp)) > 0 ){ //we read the bytes on the file, 'n'gets the amount of read bytes
									total+=n; ///counting the amount of read bytes
									send(sock, sendbuffer, n, 0);   /////se envía el buffer leído por el socket
								}
								printf("La cantidad total de bytes enviados es: %d \n",total);
							    printf("Termine de enviar la imagen\n");
								fclose(fp);
							}///end else of the section that sends the image
						}
						else{ /////'X' was wrote to get out of the chat
							chat =false;
							printf("Saliendo del chat\n\n");
						}					

						memset(msjFinal,'\0',sizeof(msjFinal));//clean buffers to future usages 
						memset(message,'\0',sizeof(message));
					}//end of getting a file descriptor input
				} //end of writing and sending a normal msj or a image
			} // end of chat
		}///// end of option 3
		
		if(option==4){}	
	}///end while
	return NULL;
}///end function menu



/////////////////////// function to manage input on the socket with a thread  ////////////////////////////////////
void *sockets(){
	while(1){
		r_set = all_set;
		select(maxfd, &r_set, NULL, NULL, &tv);//check to see if we can read from sock

		if(FD_ISSET(sock, &r_set)){//check to see if we can read from sock
		
			if( recv(sock , server_reply , 256 , 0) < 0){//Receive a reply from the server. We need to check what has been received!
				puts("recv failed");
			}	
			
			if(server_reply[0]=='1'){// if the first character is a '1' it means that another socket has been connected to the server
				printf("Cliente conectado: ");
				for(int y=0;y<maxConectados;y++){ //we save the client name to know the available clients
					if( conectados[y]=='\0'){
						conectados[y]=server_reply[1];
						break;
					}
				}
			}///end of saving the new client connected to the system
			
			if(server_reply[0]=='X'){ //if the first character is a 'X' a client has been disconnected to the system and we gotta delete it from the register we have
				printf("Un cliente se ha desconectado:");
				for(int y=0;y<maxConectados;y++){
					if( conectados[y]==server_reply[1]){
						conectados[y]='\0';
						break;
					}
				}	
			}/// end of deleting a client that has been disconnected from the system
			
			/////this segment is to receive an image
			if(server_reply[1]=='?'){
				memset(server_reply,'\0',sizeof server_reply);
				printf("Voy a recibir una imagen---");
				recv(sock,server_reply,sizeof(server_reply),0);/// here we receive the size of the image in bytes
				printf("Tamano de la imagen por recibir: %s\n",server_reply);///si se recibe el tamano de la imagen
				imageSize=atol(server_reply);  /// from str to long
				
				FILE* fp = fopen( "2.bmp", "wb");
				tot=0;
				if(fp != NULL){
				    while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size.
				    	b=recv(sock, buff, 1,0);// receive the bytes, 'b' saves the amount of bytes received
				        tot+=b; // counting the amount of bytes receives
				        fwrite(buff, 1, b, fp);  //writing the file
				    }
					printf("Ya termine de recibir \nBytes recibidos: %d\n",tot);
				    
				    if (b<0)
				       perror("Receiving");
				    
				    fclose(fp);
				} 	//end of writing the image received
				else{
				    perror("File");
				}
			}//end of receiving and writing an image.
			
			else{ ////finaly, receiving a normal msj
				for (int i=1;server_reply[i]!='\0';i++){ //server_reply[0] has the recipient client (client name who is going to receive the msj)
					printf("%c", server_reply[i]);
				}
			}
			
			printf("\n");
			memset(buff,'\0',sizeof(buff)); ///clean buffers for futures usages.
			memset(server_reply,'\0',sizeof server_reply);
		}
	}
}
