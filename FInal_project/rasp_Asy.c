#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <sys/select.h>
#include <arpa/inet.h> //inet_addr

#define COMPLETE 0
#define MAX 5

int serial_port,maxfd;
fd_set all_set, r_set; //file descriptor table to use on select()
struct timeval tv;
int msjSize;
long imageSize;
int n,wc,total,rc;
int num_bytes;
int contador;
int reenvios = 0;
//message buffer related delcartions/macros
static int inbuf; // how many bytes are currently in the buffer?
static int room; // how much room left in buffer?
static char *after; // pointer to position after the received characters
char message[100] ;
unsigned char imagen[20];
int buffer_message(char * message);
char buff_in[100];
unsigned char buff_aux[21];
char bandera [3];
float porcentaje;
bool calentando=true;
char emisor;

int lost;

long error;
long sentData;
long totalData;
/////////////////Code to receive from keyboard ///////////////////////
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
//////////////////////////////////////////////////////

void *sendMSJ();
void *receiveMSJ();

// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
int main(){

	int descriptor= socket(AF_INET , SOCK_STREAM , 0);//zero means TCP protocol
	if (descriptor== -1)
	{
		printf("Could not create socket");
	}

	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY; 
	server.sin_port = htons( 2010  );//server port	
	
	if(bind(descriptor,(struct sockaddr*)&server,sizeof(server))<0){
	perror("Error binding");
	return 1;
	}
		
	listen(descriptor,2); ///specify the maximum number of connections
	puts("Waiting for incoming connections");
	///Waiting for a client

	struct sockaddr_in client;
	int c=sizeof(struct sockaddr_in);

	int clientSock=accept(descriptor,(struct sockaddr *)&client,(socklen_t*)&c);
	
	if(clientSock<0){
		perror("Connection failed!\n");
		return 1;
	}
	puts("Connection accepted\n");
	

	serial_port = open("/dev/ttyUSB1", O_RDWR);  //////to opend a serial port, with raspberry we use  /dev/ttyS0
	maxfd=serial_port+1;
	
	//////preparing select()
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set);  ////this is important!!! we include file descriptors into a table to check out which
	FD_SET(clientSock, &all_set);    //file descriptor is requesting (STDIN_FILENO-->from keyboard,
	FD_SET(serial_port, &all_set);   //clientSock-->from socket,    serial_port-->from serial)
	r_set = all_set; //copy the table on another table
	tv.tv_sec = 1; 
	tv.tv_usec = 0;
	
	struct termios tty;

	// Read in existing settings, and handle any error
	//configurations of the serial port
	if(tcgetattr(serial_port, &tty) != 0) {
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 1;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 20;

	cfsetispeed(&tty, B19200);///setting the speed transmision of the serial port
	cfsetospeed(&tty, B19200);


	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	after = message;

	while(1){
		r_set = all_set;
        tv.tv_sec = 10;
		select(maxfd, &r_set, NULL, NULL,NULL);




////////////////////////////this segment is the connection between the phone and the raspberry using sockets//////////////////////////////////////	
		if(FD_ISSET(clientSock, &r_set)){
			
				int recibidos=recv(clientSock,buff_in,sizeof buff_in,0); // receiving msj from the socket connected to the phone
        		
				//In case that the phone has hang out we close the connection with the socket and finish the terminal
				if (recibidos==0 || buff_in[0]=='3'){
        			printf("Socket hung up\n\n");
        			close(clientSock);
        			close(serial_port);
        			break;
        		}

				else if(buff_in[0]=='2'){
					puts("Checking connection status\n\n");
					memset(buff_in,'\0',sizeof buff_in);
					sprintf(buff_in,"%ld",error);
					send(clientSock,buff_in,strlen(buff_in)+1,0);
					sleep(1);
					memset(buff_in,'\0',sizeof buff_in);
	                sprintf(buff_in,"%ld",totalData);
	                send(clientSock,buff_in,strlen(buff_in)+1,0);
					goto salida;
				}

        		recibidos=recibidos-1; //rest the NULL character
            	printf("\n\n\n\n\n\n\nMsj from phone: <<%s>> Size on bytes:%d\n\n",buff_in,recibidos);
				char header[] = "EL5522TALLERDECOMU";
                char final[] = "ENDtallerEL5522";
                char messconfi[8]="InitCon";
                char buff_mess[100];
                char buff_confirm[8];				
              //  write(serial_port, header, sizeof(header));//send header to the other endpoint to confirm connection:


			////first, we have to initiate the connection between transceivers.
			////Sending the header. If we don't have answer, it'll cancel the sending of the msj
			////initiating transmission from raspberry to the other pc through transceivers
            	while(1){
				
			        msjSize=write(serial_port,header,strlen(header));/////send header to the other endpoint to confirm connection:
					totalData+=msjSize; //suming sent bytes
					printf("Header sent: %s \n",header);
					r_set = all_set;
					tv.tv_sec = 2; ///this is the time in seconds that we are gonna wait for an answer :)
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){///timeout
						contador++; ///counting timeouts
						error+=msjSize;    ///counting errors in the transmissions
						puts("No answer");
						if(contador==MAX){/// number of timeouts is achieved, close connection
							contador=0;
							printf("ERROR. Receiver was not found\n\n");
							send(clientSock,"N",2,0);
							goto salida;////byee
						}
						continue; ///go to while and try it again
					}
                    contador = 0;
					
                    break;  ///if the function 'select()' returns a nonzero value, and positive, we have an answer on the serial port
			
				}/////end of the while
			//	totalData+=msjSize; //suming sent bytes

				send(clientSock,"S",2,0);
                // recieve confirmation from the client  (transceiver with the pc)

               	memset(buff_confirm,'\0',sizeof buff_confirm);
                int bytes_confirm = read(serial_port, &buff_confirm, sizeof(buff_confirm));///read the confirmation msj
                
                int confirmation = strcmp(messconfi, buff_confirm);////compare the header msj, it returns zero if they are the same
                if(confirmation == 0){///AWESOME!!! THEY ARE EQUAL
                
                    printf("Successful Startup Protocol!\n");
                    char tamanoMSJ[10];
                    sprintf(tamanoMSJ,"%d",recibidos);///from int to str
                    sentData=write(serial_port,tamanoMSJ,strlen(tamanoMSJ));//we send the size of the msj
				//	totalData=+sentData; //suming sent bytes
					sleep(1);
					
                	// Continue sending the message
                	int cantidad=0;
                	char seg[21];///the msj will be send and receive on this buffer, 20 bytes of data and 1 reserved to NULL
                	memset(seg,'\0',sizeof seg);
                	while(1){	///send the message till we achieve the time out
                	
   						while(cantidad<recibidos){///sending msj 20 by 20
   							memcpy(seg,buff_in+cantidad,20);
   							msjSize=write(serial_port,seg,strlen(seg));///
   							cantidad=cantidad+msjSize;
							totalData+=msjSize;
   							usleep(100000);
   						}
						

   						
   						cantidad=0;
						r_set = all_set;
						tv.tv_sec =2 ;///timeout
						
						////Now, we have to receive an answer from the other transceiver
						if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
							contador++;
							error+=msjSize;
							printf("No answer\n");
							if(contador==MAX){
							contador=0;
								printf("Receiver was not found\n\n");
							
								goto salida;////close
							}
							continue; ///go to while and try it again
						}
						
						////here, select returned a requuest from the serial port

                        contador = 0;
						
						///On the next lines, we will receive the complete msj from the other transceiver
						memset(buff_mess,'\0',sizeof buff_mess);
						while(cantidad<recibidos){ ///receiving 20 by 20
							memset(seg,'\0',sizeof seg); ///clean ALWAYS AALWAYS the buffer before reading
							bytes_confirm = read(serial_port, seg, sizeof(seg));
							cantidad=cantidad+bytes_confirm;
							strcat(buff_mess,seg);////concatenate the msj
							
						}
						
						printf("\nThe other transceiver resent the package: <<%s>>\n",buff_mess);
						
						usleep(10000);
						///comparison
	                    int confirmation =strcmp(buff_mess, buff_in);
                        if (confirmation==0){  //if they are the same
                            sentData=write(serial_port, "1", 2); // 
                            printf("Package is correct\n");// tudo bem , break this while
							//totalData+=sentData;//suming total sent bytes
                        }

                        else{
                            sentData=write(serial_port, "0", 2);// Aqui tenemos que enviar bandera
                           // totalData+=sentData;
							printf("\n\nMessage is incorrect. Retrying\n\n");
                            reenvios++;
							
                            if(reenvios == MAX){
                                printf("\nAmount of attempts have been achieved.\n Closing\n\n");
                                reenvios = 0;
                                goto salida;
                            }        
                            continue; /// go to the while and attempt it againg
                        }
                        
                        break;
				
					}////end of while
					
					

					///////the message is an image, first receive it from the phone throug sockets and resend it throug the transceiver by serial port
	
					if(buff_in[2]=='~'){
						
						//////////first, receive the image from the socket
						send(clientSock,buff_in,strlen(buff_in)+1,0);///send reply to the phone 
						memset(message,'\0',sizeof(message)); 
						recv(clientSock,message,sizeof(message),0);/// here we receive the size of the image in bytes
						printf("Image size we are gonna receive: %s\n",message);///si se recibe el tamano de la imagen
						long imageSize= atol(message);  /// from str to long
						FILE* fp = fopen( "phoneRasp.bmp", "wb");
						int tot=0;
						int b=0;
						if(fp != NULL){
							while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size.
								b=recv(clientSock, message, 1,0);// receive the bytes, 'b' saves the amount of bytes received
								tot+=b; // counting the amount of bytes receives
								fwrite(message, 1, b, fp);  //writing the file
							}
							printf("Total image has been received \nReceived Bytes: %d\n\n",tot);		
						} 	//end of writing the image received
						else{
							perror("File");
						}
						fclose(fp);
						
					
//****						/////send the image throung trasnceiver by serial port
						memset(buff_in,'\0',sizeof(buff_in));
						memset(message,'\0',sizeof(message)); //se limpia la memoria del msj
						printf("Sending image through transceiver....... \n");
						FILE *fp2 = fopen("phoneRasp.bmp", "rb");   ///// open the image that we are gonna send!
						if(fp2 == NULL){
							perror("File");
						}
						///we need to send the size of the file 
						int size=0;
						fseek(fp2,0,SEEK_END);///esto es para informar el tamaño total del file, puntero se ubica al final
						size=ftell(fp2); //se guarda posicion del puntero
						sprintf(message,"%d",size);///de long a str, se guarda en el buffer a enviar
						usleep(1000000);
						
						write(serial_port,message,strlen(message));// sending the file's lenght
						
						fseek(fp2,0,SEEK_SET);//se vuelve a colocar el puntero al inicio del file
						total=0;
						contador=0;  
												
						//read(serial_port,buff_in,sizeof(buff_in));
					
						unsigned char buffer[20];
						memset(buffer,'\0',sizeof(buffer));
						memset(buffer,'\0',sizeof(imagen));
						
						while( (n = fread(imagen, 1, sizeof imagen, fp2)) > 0 ){ //we read the bytes on the file, 'n'gets the amount of read bytes
														
							while(1){///sending the packets
								wc=write(serial_port, imagen,n); //send a packet and check for an answer
								totalData+=wc;//suming total bytes
								r_set = all_set;
								tv.tv_sec = 2; 
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){ 
									error+=wc; ////sum, it means we didn't receive and have to resend
									contador++; 
									printf("No answer from the receiver\n");
									if(contador==10){//we try that amount of times
										contador=0;
										printf("No answer. We lost connection\nClosing....\n\n");
										fclose(fp2);///close the file
										goto salida; ///go to exit
									}
									continue; ///go to while and try it again
								}
								///if we are here is because we got an answer from the receiver

								contador=0;
								rc=read(serial_port,buffer,n);////read the resent message from the receiver to check if it is correct
								if(memcmp(imagen,buffer,n)==0){ //if they are equal, send a '1' to the receiver to confirm packet and break the while
									sentData=write(serial_port,"1",3);
									//totalData+=sentData;
									break;
								}
								sentData+=write(serial_port,"N",3); /// the resent packet is incorrect, try again
								error+=rc;
								printf("Incorrect package. Retrying....\n");
								lost++;
								if(lost==20){
									lost=0;
									fclose(fp2);
									printf("Error. Lost connection\n\n");
									goto salida;
								}
								
							}
							//if we are here is because the packet has been sent correctly 
							total=total+n;
		                    porcentaje = (float)total/size * 100;
		                    printf("Read:%d ,Written: %d ,Received:%d , total:%d , %3.2f %%\n\n",n,wc,rc, total, porcentaje);
						}
						
						printf("\nImage has been sent sucessfully. Total Bytes: %d\n",total);
					
						if(fclose(fp2)){
							printf("Error closing the file\n");	
						}
					}
                    // EXIT protocol
                    printf("\n\n####Closing protocol####\n");
                    usleep(500000);
                    write(serial_port, final, sizeof(final)); // Manda el mensaje para protocolo de cierre
					
                    // FIn de protocolo de cierre
    
				}
				else{
					printf("The header is wrong\nUnsuccessful Startup Protocol\nClosing\n\n\n");
				}
			
			salida:
				FD_CLR(serial_port, &r_set);///nos olvidamos que algun momento recibimos del serial port
		}
		
		
		
		
		
		
		
		
        // -------------------------------------------- Receive part of the program ----------------------------
		
		if(FD_ISSET(serial_port, &r_set)){//check to see if we can read from transceiver

			char verify[] = "EL5522TALLERDECOMU";
			char recibe[100];
			char registro[100];
			memset(registro,'\0',sizeof registro);
			num_bytes = read(serial_port, &recibe, sizeof(recibe));////we are receiving the verified msj 'EL5522TALLERCOMU'

			int counter_verify = strcmp(recibe, verify);
			if(counter_verify == 0){ // Verify that header is correct
		
				int cont_reenvios = 0;

				printf("\n\n\n\n-------Successful Startup Protocol-------\n\n");
				char init[] = "InitCon";
				write(serial_port, init, strlen(init)); // send message to know that the connection was initialized
				
				
				//// Here we are receiving the package ///////////////////////

				memset(recibe, '\0', sizeof(recibe));
				read(serial_port,recibe,sizeof recibe);//receive msj size
				int tamanoMSJ=atoi(recibe);
			
				int cantidad=0;
				char segment[100];				
				memset(recibe, '\0', sizeof(recibe));
				
				////leyendo con timeout
correc:
				while(true){///reading 
					r_set = all_set;
					tv.tv_sec = 2; 
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){////is there sth to read? NO
						contador++;
						printf("No answer\n");
						if(contador==MAX){
							contador=0;
							printf("Error. Lost connection\n\n");
							memset(recibe, '\0', sizeof(recibe));
							goto salidaReceiving;
						}
						continue;
					}
					else{//There is sth to read!! Read it!!
						contador = 0; // restart the counter
						
						while(cantidad<tamanoMSJ){///receiving 20 by 20
							memset(segment,'\0',sizeof segment);
							num_bytes=read(serial_port,segment,sizeof(segment));//
							cantidad=cantidad+num_bytes;
							strcat(recibe,segment); ///concatenating package
						}
						cantidad=0;						

						break;///we have received all the package
					}
				}
				
				////////resent the page to verify ////////
				
///this a test	char buff_temp[6] = "Nuevo"; // Este es para probar
//			    write(serial_port, buff_temp,sizeof(buff_temp)); //prueba para enseñarle a papi que sirve
//if you want to test it, uncomment the 2 previous lines and comment the next while.
				msjSize=0;
				while(cantidad<tamanoMSJ){//resending the msj 20 by 20
					memcpy(segment,recibe+cantidad,20);
					msjSize=write(serial_port,segment,strlen(segment));///envia msj
					cantidad=cantidad+msjSize;
					usleep(100000);					
				}
				totalData+=cantidad;
		
				// Receiving the flag to verify
				read(serial_port, buff_aux, sizeof(buff_aux));

				if (buff_aux[0] == '0'){ // If the package was wrong
					
					reenvios++; // count it
					error+=cantidad; //count the times we have to resent the package
					if(reenvios == MAX){ 		
						printf("\nERROR.Maximun attempts has been achieved.\n\nClosing...\n");
						reenvios = 0;
						goto salidaReceiving;					
					}
					
					printf("\n###The package is incorrect. Resending the package###\n\n");
					goto correc; // go and send the package again;
				}

				
				printf("\nPackage is correct: << %s>>\n",recibe);
				strcpy(registro,recibe);
				

				if(recibe[2]=='~'){ // we will receive an image
					memset(recibe,'\0',sizeof recibe);
					printf("Receiving an image through a transceiver \n");
					//usleep(100000);
					read(serial_port, recibe,sizeof recibe);///read the lenght of the file
					imageSize = atol(recibe);
					printf("Image size: %ld\n",imageSize);//// :)
					
					//usleep(100000);
					//write(serial_port,'\0',2);
					FILE* fp = fopen( "transRasp.bmp", "wb");
					int tot=0;
					int b;
					lost=0;
					usleep(10000);
					if(fp != NULL){
						//printf("\n\nGrande palomilla\n\n");
				
						while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size
							while(1){ ////in this while,we receive a package of 20 character, resent it to the emisor and wait for a flag to verify is the package was correct
								r_set = all_set;
								tv.tv_sec = 2; //time we will wait to receive a package
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){

									contador++;
									printf("No package received\n");
									if(contador==10){
										contador=0;
										printf("Error. Lost connection\nClosing..\n");
										fclose(fp); ///closing image file
										goto salidaReceiving; ///exit
									}
									continue;
								}
								// if we are here, we have a package to receive
								contador=0;
								b=read(serial_port, buff_aux, sizeof(buff_aux));// receive the bytes, 'b' saves the amount of bytes received
								//usleep(100000);
								wc=write(serial_port,buff_aux, b);///resent the exact same package
								//usleep(100000);
								
								r_set=all_set;
								tv.tv_usec = 10000;
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
									continue;
								}
								read(serial_port, bandera, sizeof(bandera));//receiving the flag to verify 
								//usleep(100000);
								
								totalData+=wc;
								if(bandera[0]=='1'){///if the flag is 1 it means the package is correct and break the while

									break;
								}
								error+=wc;
								///if the flag is not '1' it is considered that the package was wrong and have to receive it again :c sad						
																
								lost++;
								
								if(lost==10){
									lost=0;
									fclose(fp);
									printf("All packages are incorrect. Closing...\n");
									goto salidaReceiving;
								}
								printf("Package incorrect. Retrying...\n");




							}
					
							tot+=b; // counting the amount of bytes receives
							porcentaje = (float)tot/imageSize *100;
							printf("Received %d ,Forwarded:%d ,Total: %d ,Percentage: %3.2f %%\n\n",b,wc,tot, porcentaje);
							fwrite(buff_aux, 1, b, fp);  //writing the file
						
						}
						printf("Image received\nTotal bytes: %d\n",tot);
				    
						if(fclose(fp)!=0){
							printf("Error. Problem closing the image file\n");
					}
						printf("Image closed");
					} 	//end of writing the image received
					else{
						perror("File");
					}
					
					
					
					
					////Now we send the image to the phone
					send(clientSock,registro,sizeof registro,0);///into 'registro' we have the message "~", send it!!
					printf("\n\nSending image to the phone\n");
					recv(clientSock,message,sizeof(message),0); //trash to unblock
					unsigned char buffer[20];
					
					FILE *fp3 = fopen("transRasp.bmp", "rb");   ///// open the image that we are gonna send!
			
					if(fp3 == NULL){
						perror("File");
						//return 2;
					}
					///we need to send the size of the file 
					long size=0;
					fseek(fp3,0,SEEK_END);///set pointer at the end of the file
					size=ftell(fp3); //it saves the amount on bytes/
					memset(message,'\0',sizeof(message)); //
					sprintf(message,"%ld",size);/// from long to str. we need to send it
					printf("Image size to send: %ld\n",size);
					send(clientSock, message, sizeof(message) , 0);// image size is send
					fseek(fp3,0,SEEK_SET);//set the pointer at the beginning
					int n;
					int total=0; // to count the amount of bytes sent
					
					while( (n = fread(buffer, 1, sizeof(buffer), fp3)) > 0 ){ //we read the bytes on the file, 'n'gets the amount of read bytes
						total+=n; ///counting the amount of read bytes
						send(clientSock, buffer, n, 0);   /////
					}
					
					printf("Image has been sent.\nTotal bytes sent:%d \n",total);
					
					fclose(fp3);//close the file
					
					
				} // ceerando buff_in = ~
				else{
					send(clientSock,recibe,sizeof recibe,0);
					printf("Message forwarded to phone by socket\n\n\n");
				}

				// Closing of the protol
				printf("####Exit Protocol####\n");
				memset(recibe, '\0', sizeof(recibe));
				

				
				while(true){///leelo bien
					r_set = all_set;
					tv.tv_sec = 2; 
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
						
						contador++;
						printf("Waiting for exit message\n");
						if(contador==3){
							contador=0;
							printf("Lost connection\nClosing\n\n");
							goto salidaReceiving;
						}
						continue;
					}
					else{
						contador = 0; // para reiniciar contador
						num_bytes=read(serial_port,recibe,sizeof(recibe));//
						int confirmation = strcmp(recibe, "ENDtallerEL5522"); // Comparo con ENDtallerEL5522
						if(confirmation == 0){
							printf("Exit message is correct:  %s\n", recibe);
							break;
						}
						else
							printf("\n\nExit message is incorrect, %s\n\n", recibe);
					} // closing the verification						
								
					break;
				}
		
				
				
				} 
				
				
				salidaReceiving:
					printf("-----Connection finished-----\n\n\n");
					FD_CLR(clientSock, &r_set);
					FD_CLR(serial_port, &r_set);
					memset(recibe, '\0', sizeof(recibe));
			}//cerrando si llego mensaje nuevo


	}//closing while
}





