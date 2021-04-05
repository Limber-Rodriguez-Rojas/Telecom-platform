#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>


#define COMPLETE 0

char message[256] ;
int buffer_message(char * message);///buffer where we will save the input msj from keyboard.
static int inbuf; // how many bytes are currently in the buffer?
static int room; // how much room left in buffer?
static char *after; // pointer to position after the received characters

////////////////////receive from keyboard //////////////////////////////

int find_network_newline(char * message, int bytes_inbuf){
    int i;
    for(i = 0; i<inbuf; i++){
        if( *(message + i) == '\n')
        return i;
    }
    return -1;
}



int buffer_message(char * message){
    int bytes_read = read(STDIN_FILENO, after, 256 - inbuf);
	short flag = -1; // indicates if returned_data has been set 
    inbuf += bytes_read;
    int where; // location of network newline
    // Step 1: call findeol, store result in where
    where = find_network_newline(message, inbuf);
    if (where >= 0) { // OK. we have a full line
		
        // Step 2: place a null terminator at the end of the string
        char * null_c = {'\0'};
        memcpy(message + where, &null_c, 1); 

        // Step 3: update inbuf and remove the full line from the clients's buffer
        memmove(message, message + where + 1, inbuf - (where + 1)); 
        inbuf -= (where+1);
        flag = 0;
    }
    // Step 4: update room and after, in preparation for the next read
    room = sizeof(message) - inbuf;
    after = message + inbuf;

    return flag;
}
///////////////////////////////////////////////////////////////////////////////


void printMenu(){
      puts("On the next lines you can find the options\nP-Send msj to phone + your msj\n(A or B)-Send msj to # + your msj\n?-How much do I have?\n1-Top up your credit account\n2-Exit\n\n");
}



int main(int argc, char *argv[]){
	

	int descriptor=socket(AF_INET,SOCK_STREAM,0);  ////create a socket(domain,type,protocol), this function returns a integer that is the file descriptor of the socket
	if(descriptor==-1){
		printf("Error creating the socket");
	}
	puts ("Socket created");
	struct sockaddr_in server; ///struct where the information of the socket is saved
	
	///The call to memset() ensures that any parts of the structure thar we don't explicitly ser contain zero.
	
	server.sin_addr.s_addr=inet_addr("52.14.142.83");  ////here you put the server's IP,actually this the AWS IP
	//Also can use inet_addr("ip numbers and dots") or inet_ntoa("ip numbers and dots")
	server.sin_family=AF_INET;
	server.sin_port=htons(2010); // here you specify the port number

	if(connect(descriptor, (struct sockaddr *)&server, sizeof(server))  <0){   
		perror("connect faliled. Error");
		return 1;
	}
	puts("Connected\n");
	puts("Thanks for using RIVERACOM's communication system\n");
	printMenu();

	fd_set all_set, r_set; //file descriptors to use with the function select()
	struct timeval tv;
	tv.tv_sec = 3; 
	tv.tv_usec = 0;
	int maxfd=descriptor+1;
	//////preparing select()
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set);
	FD_SET(descriptor, &all_set);
	r_set = all_set;
	char buffer[100]; 
	int check;
	after = message;
	while(1){

cerrar:
		r_set = all_set;
        tv.tv_sec = 1;
		if(select(maxfd, &r_set, NULL, NULL, &tv)>0){
			
			if(FD_ISSET(STDIN_FILENO, &r_set)){
				
				if(buffer_message(message) == COMPLETE){////if we have a input from the keyboard on the file descriptor STDIN_FILENO

					if(message[0]=='2'){
						printf("Thanks for using RIVERACOM's services.\n\n\n\n");
						break;
					}



					if( !(message[0]=='1' || message[0]=='2'  || message[0]=='A' || message[0]=='B' || message[0]=='P' || message[0] == '?')){
		                 puts("Option is not possible\n\n");
		                 printMenu();
		                 goto cerrar;
		         	}

					//We are sending the msj of the next line
					send(descriptor,message,strlen(message)+1,0); //send(descriptor socket,msj,tamaño de msj,0 por default)
					//puts(" ");
					//if the msj is not a normal text, we are sending an Image
					
					memset(buffer,'\0',sizeof buffer);//cleaning the buffer
					int recibidos=recv(descriptor,buffer,sizeof buffer,0);// receive the msj and save it on buffer
					//printf("%s\n\n",buffer);
					char notificando[100];
					if(buffer[0]=='A' || buffer[0]=='B'){
						message[2]='q';
						int copiando=strlen(buffer)-2;
						memset(notificando,'\0',sizeof notificando);
						memcpy(notificando,buffer+2,copiando);
					}
					printf("Notification: %s\n\n\n\n",notificando);
					
					if(message[2]=='~'){ 
						printf("Sending an image\n");					
						
						FILE *fp = fopen("imagen.bmp", "rb");   ///// open the image that we are gonna send!
						int tot=0;
						int b;

						if(fp == NULL){
							perror("File");
							
						}
						///we need to send the size of the file 
						long size=0;
						fseek(fp,0,SEEK_END);///set the pointer on the end of the file to know the size on bytes.
						size=ftell(fp); //save the pointer position
						memset(buffer,'\0',sizeof(buffer)); //clear buffer
						sprintf(buffer,"%ld",size);///from long to string, it is saved on buffer
						send(descriptor, buffer, sizeof(buffer) , 0);// the size of the image has been sent
						printf("Image size sent\n");
						fseek(fp,0,SEEK_SET);//set the pointer on the begining of the file
						

						////Next: read the file in bytes and send them!
						int n;
						int total=0; // to count the amount of bytes sent
		
						while( (n = fread(buffer, 1, sizeof(buffer), fp)) > 0 ){ //we read the bytes on the file, 'n'gets the amount of read bytes
							total+=n; ///counting the amount of read bytes
							send(descriptor, buffer, n, 0);   /////se envía el buffer leído por el socket
						}
						printf("Image has been sent\nTotal bytes sent: %d \n\n\n\n",total);
						
						fclose(fp); ///closing the file
					}///end else of the section that sends the image
				}
				FD_CLR(descriptor, &r_set);	
				memset(message,'\0',sizeof message);
				memset(buffer,'\0',sizeof(buffer)); //clear buffer
			}
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


			if(FD_ISSET(descriptor, &r_set)){ /// receiving something on the socket
				memset(buffer,'\0',sizeof buffer);//cleaning the buffer
				int recibidos=recv(descriptor,buffer,sizeof buffer,0);// receive the msj and save it on buffer
				
				//getting the IF of the emisor and the message on different buffers
				char emisor=buffer[1];//Save the ID of the emisor
				char aux[250];
				memset(aux,'\0',sizeof aux);
				memcpy(aux,buffer+2,recibidos-2);//saving  the msj on aux
				if(emisor==' ')
					emisor='S';
				printf("Msj received from %c: %s \n\n\n",emisor,aux);
				
				FD_ZERO(&r_set);
				
			
				if(buffer[2]=='~'){ // We are going to receive an image
					send(descriptor,buffer,strlen(buffer),0);
					memset(buffer,'\0',sizeof buffer);
					printf("---Receiving an Image---\n");
					recv(descriptor,buffer,sizeof(buffer),0);/// here we receive the size of the image in bytes
					printf("Image size: %s\n",buffer);
					int imageSize= atoi(buffer);  /// from str to long
					
					FILE* fp = fopen( "awsUser.bmp", "wb");
					int tot=0;
					int b;
					if(fp != NULL){
						while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size.
							b=recv(descriptor, buffer, 1,0);// receive the bytes, 'b' saves the amount of bytes received
							tot+=b; // counting the amount of bytes receives
							fwrite(buffer, 1, b, fp);  //writing the file
						}
						printf("Received bytes: %d\n\n\n\n",tot);
						if (b<0)
						   perror("Receiving");
						fclose(fp);
					} 	//end of writing the image received
					else{
						perror("File");
					}
				}//end of receiving and writing an image.
				memset(buffer,'\0',sizeof buffer);
				memset(message,'\0',sizeof message);
				FD_CLR(descriptor, &r_set);	
			}
			FD_ZERO(&r_set);
		}
	
	}
	close(descriptor);
	return 0;

}
