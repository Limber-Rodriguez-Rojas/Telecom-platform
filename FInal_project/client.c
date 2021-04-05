#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>


#define COMPLETE 0
#define row 25 /// to graph the bars
#define col 210




char message[256] ;
int buffer_message(char * message);
static int inbuf; // how many bytes are currently in the buffer?
static int room; // how much room left in buffer?
static char *after; // pointer to position after the received characters

char strError[100];
long error,totalData;

char graf1[]="Total Data"; //10 char  ///to graph bar
char graf2[]="Total Error"; //11 char



void graficar(long totalData, long error){
	
	char data[7];
	char fallos[7];
	sprintf(data,"%ld",totalData);
	sprintf(fallos,"%ld",error);
	int a=strlen(data);
	int b=strlen(fallos);
	int restaA=col-10-1-a;
	int restaB=col-11-1-b;

	int divA=(int)totalData/100;
	int divB=(int)error/100;
	if(divA>=208){
		divA=207;
	}

	if(totalData<100 || divA==1){
		divA=2;
	}

	if(divB>=208){
		divB=207;
	}	
	if(error<=100 || divB==1){
		divB=2;////we want to print from position 2
	}

	for(int i=0;i<row;i++){
		
		for(int k=0;k<col;k++){

			///printing the first and the last lines
			if(i==0 || i==(row-1)){
				printf("#");
			}


			///printing total data
			else if(i==5 && k==2){
					printf("%s:%ld",graf1,totalData);
			}
			else if(i==5 && k>=3 && k<=restaA){
					if(k==restaA){
						printf("#");	
						break;
					}
					printf(" ");
			}

			else if(i>5 && i<10 && k>1 && k<=divA){
				printf("W");
			}




			///printing error data
			else if(i==15 && k==2){
				printf("%s:%ld",graf2,error);
			}
			else if(i==15 && k>=3 && k<=restaB){
					if(k==restaB){
						printf("#");
						break;
					}
					printf(" ");
			}

			else if(i>15 && i<20 && k>1 && k<=divB){
				printf("X");
			}




			else{
				if(k==0 || k==(col-1))
					printf("#");
				else
					printf(" ");
			}
		}
		puts(" ");
		
		


	}

}



/////code to receive and save text from the keyboard/////////////////////
int find_network_newline(char * message, int bytes_inbuf){
    int i;
    for(i = 0; i<inbuf; i++){
        if( *(message + i) == '\n')
        return i;
    }
    return -1;
}


void printMenu(){
	puts("On the next lines you can find the options\nA-Send msj to A + your msj\nB-Send msj to B + your msj\n?-How much do I have?\n1-Top up your credit account\n2-Check connection\n3-Exit\n\n");
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



int main(int argc, char *argv[]){
	puts("\n\n\nThanks for using RIVERACOM's communication system\n\n\n");

	int descriptor=socket(AF_INET,SOCK_STREAM,0); 
	if(descriptor==-1){
		printf("Error creating the socket");
	}

	struct sockaddr_in server;	
	server.sin_addr.s_addr=inet_addr("127.0.0.1");  ////here you put the server's IP
	//Also can use inet_addr("ip numbers and dots") or inet_ntoa("ip numbers and dots")
	server.sin_family=AF_INET;
	server.sin_port=htons(2010); // here you specify the port number

	if(connect(descriptor, (struct sockaddr *)&server, sizeof(server))  <0){   
		perror("connect faliled. Error");
		return 1;
	}
	
	printMenu();


	///preparing select()
	fd_set all_set, r_set;
	struct timeval tv;
	tv.tv_sec = 3; 
	tv.tv_usec = 0;
	int maxfd=descriptor+1;

	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set);
	FD_SET(descriptor, &all_set);
	r_set = all_set;
	char buffer[100]; 
	int check;
	after = message;
	while(1){
cerrar:///this applies when an invalid option is introduced or connection between trasnceivers was not possible
		r_set = all_set;
        tv.tv_sec = 2;
		if(select(maxfd, &r_set, NULL, NULL, &tv)>0){
			if(FD_ISSET(STDIN_FILENO, &r_set)){///receiving something from the keyboard
				if(buffer_message(message) == COMPLETE){
				
					if(message[0]=='1'){
					}

		            if(message[0] == '?'){
		                printf("\nAsking for my money....\n");
		            }

					if(message[0]=='2'){
						puts("Checking connection status!\n\n");
						send(descriptor,message,strlen(message)+1,0);
						memset(strError,'\0',sizeof strError);
						recv(descriptor,strError,sizeof strError,0);
						error=atol(strError);
						
						memset(strError,'\0',sizeof strError);
						recv(descriptor,strError,sizeof strError,0);
						totalData=atol(strError);

						printf("Amount of total packages %ld \n",totalData);
						printf("Amount of resent packages %ld \n\n\n",error);
						
						graficar(totalData,error);
						
						goto cerrar;
					}
						
					if(message[0]=='3'){
						printf("\n\n\nThanks for using RIVERACOM's services.\n\n\n\n");
						send(descriptor,message,strlen(message)+1,0);
						break;	
					}
					if(message[0]=='P' || !(message[0]=='1' || message[0]=='2' || message[0]=='3' || message[0]=='A' || message[0]=='B' || message[0] == '?')){
						puts("Option is not possible\n\n");
						printMenu();
						goto cerrar;
					}	
					
					send(descriptor,message,strlen(message)+1,0);///send the msj to the raspberry trought the socket, 
					puts("Msj sent\n\n\n");
					memset(buffer,'\0',sizeof(buffer));
					recv(descriptor,buffer,sizeof buffer,0);///
					if(buffer[0]=='N'){
						puts("ERROR. Connection not stablished between transceivers\n\n");
						goto cerrar;
						
					}
					
					memset(buffer,'\0',sizeof(buffer));
					//if the msj is not a normal text, we are sending an Image
					if(message[2]=='~'){ 			
						recv(descriptor,buffer,sizeof buffer,0);

						FILE *fp = fopen("sc.bmp", "rb");   ///// open the image that we are gonna send!
						int tot=0;
						int b;

						if(fp == NULL){
							perror("File");
							
						}
						///we need to send the size of the file 
						long size=0;
						fseek(fp,0,SEEK_END);///place the pointer file in the end of the file
						size=ftell(fp); //save the position on bytes (size of the file)
						memset(buffer,'\0',sizeof(buffer)); //
						sprintf(buffer,"%ld",size);///from long to str
						send(descriptor, buffer, sizeof(buffer) , 0);// sending the image size
						fseek(fp,0,SEEK_SET);//set the pointer at the begining of the file 

						////time to read and send the image
						int n;
						int total=0; // to count the amount of bytes sent
						puts("Sending an image to raspberry\n");		
						while( (n = fread(buffer, 1, sizeof(buffer), fp)) > 0 ){ //we read the bytes on the file, 'n'gets the amount of read bytes
							total+=n; ///counting the amount of read bytes
							send(descriptor, buffer, n, 0);   /////send the bytes read and saved into the socket
						}
						printf("Total bytes sent: %d \n\n\n",total);
						fclose(fp); ////closing the file
					}///end else of the section that siends the image
				}
			
			}
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


			if(FD_ISSET(descriptor, &r_set)){
				memset(buffer,'\0',sizeof buffer);
				int recibidos=recv(descriptor,buffer,sizeof buffer,0);//bloquea
				char emisor=buffer[1];
				if(emisor==' ')
					emisor='S';
				char aux[250];
				memset(aux,'\0',sizeof aux);
				memcpy(aux,buffer+2,recibidos-2);
				printf("Msj received from %c: %s \n\n",emisor,aux);
				

				if(buffer[2]=='~'){ // We are going to receive an image
					check=write(descriptor,buffer,strlen(buffer));
					memset(buffer,'\0',sizeof buffer);
					printf("Receiving an Image\n");
					recv(descriptor,buffer,sizeof(buffer),0);/// here we receive the size of the image in bytes
					printf("Image size: %s\n",buffer);///si se recibe el tamano de la imagen
					int imageSize= atoi(buffer);  /// from str to long
					
					FILE* fp = fopen( "raspPhone.bmp", "wb");
					int tot=0;
					int b;
					if(fp != NULL){
						while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size.
							b=recv(descriptor, buffer, 1,0);// receive the bytes, 'b' saves the amount of bytes received
							tot+=b; // counting the amount of bytes receives
							fwrite(buffer, 1, b, fp);  //writing the file
						}
						printf("Image has been received\nTotal bytes received: %d\n\n\n",tot);
						if (b<0)
						   perror("Receiving");
						fclose(fp);
					} 	//end of writing the image received
					else{
						perror("File");
					}
				}//end of receiving and writing an image.
					
			}
		}
		
		FD_ZERO(&r_set);
		memset(message, '\0',sizeof(message));
		
	}
	close(descriptor);
	
	return 0;

}
