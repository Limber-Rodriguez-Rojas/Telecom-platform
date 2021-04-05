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
fd_set all_set, r_set; //file descriptors to use on select()
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
int lost;

int buffer_message(char * message);
char buff_in[100];
unsigned char buff_aux[21];
char bandera [3];
float porcentaje;
bool calentando=true;
char socketMessage[100];
char error[5];

int find_network_newline(char * message, int bytes_inbuf){
    int i;
    for(i = 0; i<inbuf; i++){
        if( *(message + i) == '\n')
        return i;
    }
    return -1;
}
/////////////////////////receive from keyboard//////////////////////////


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


void *sendMSJ();
void *receiveMSJ();

// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
int main(){
	int descriptor=socket(AF_INET,SOCK_STREAM,0);
	if(descriptor==-1){
		printf("Error creating the socket");
	}
	puts ("Socket created");
	struct sockaddr_in server;
	server.sin_addr.s_addr=inet_addr("52.14.142.83");  ///can use inet_addr("ip numbers and dots")
	server.sin_family=AF_INET;
	server.sin_port=htons(2010); // here you specify the port number

	if(connect(descriptor, (struct sockaddr *)&server, sizeof(server))  <0){   
		perror("connect faliled. Error");
		return 1;
	}
	puts("Connected\n");


	serial_port = open("/dev/ttyUSB0", O_RDWR);  //////en raspberry pi   /dev/ttyS0
	maxfd=serial_port+1;
	
	//////preparing select()
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set);
	FD_SET(serial_port, &all_set);
	FD_SET(descriptor,&all_set);
	r_set = all_set;
	tv.tv_sec = 1; 
	tv.tv_usec = 0;
	
	struct termios tty;

	// Read in existing settings, and handle any error
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

	cfsetispeed(&tty, B19200);
	cfsetospeed(&tty, B19200);


	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}
	after = message;

	while(1){
		r_set = all_set;
        tv.tv_sec = 2;
		select(maxfd, &r_set, NULL, NULL, NULL);
		
		////NOTE:  We can receive a message by sockets(from AWS SERVER) or by the serial port(from a transceiver)	


		////////// in this segment, 'descriptor' is who listens the calls from the socket//////
		if(FD_ISSET(descriptor, &r_set)){
            	///First, receive the package that was sent by AWS
				memset(socketMessage,'\0',sizeof socketMessage);
            	int recibidos=recv(descriptor,socketMessage,sizeof socketMessage,0);
            	recibidos=recibidos-1;
            	printf("Package received from AWS server: <<%s>>\nPackage size: %d",socketMessage,recibidos);  ////PA   PB   + msj
	
				error[0]=socketMessage[1];
				error[1]=socketMessage[1];
				error[2]='^';               	
       	
				///Second, send the package through transceiver
				char header[] = "EL5522TALLERDECOMU";
                char final[] = "ENDtallerEL5522";
                char messconfi[8]="InitCon";
                char buff_mess[100];
                char buff_confirm[8];				
              //  write(serial_port, header, sizeof(header));//send header to the other endpoint to confirm connection:
				printf("\n\n-------Startup Protocol-------\n");

            	while(1){///send the header key till time out is achieved
			        msjSize=write(serial_port,header,strlen(header));/////send header to the other endpoint to confirm connection:
					printf("Header sent: %s \n",header);
					r_set = all_set;
					tv.tv_sec = 2; 
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){///timeout de 3 veces
						contador++;
						printf("No answer\n");
						if(contador==MAX){
						contador=0;
							printf("ERROR. Receiver was not found\n\n");
					
							///buff_in= "AA^" "BB^"  receptor/emisor
							send(descriptor,error,sizeof error,0);

							goto salida;
						}
						continue;
					}
                    contador = 0;
                    break;
			
				}///Closing while

				
                // recieve confirmation of the client
               	memset(buff_confirm,'\0',sizeof buff_confirm);
                int bytes_confirm = read(serial_port, &buff_confirm, sizeof(buff_confirm));///recieve InitCon
                int confirmation = strcmp(messconfi, buff_confirm);		///compare it
                if(confirmation == 0){
            		printf("Successful Startup Protocol!\n");
					///time to send the package
            		char tamanoMSJ[10];
            		sprintf(tamanoMSJ,"%d",recibidos);
            		write(serial_port,tamanoMSJ,strlen(tamanoMSJ));///send msj size
        
					sleep(1);
					
                	                	
                	int cantidad=0;
                	char seg[21];	
                	memset(seg,'\0',sizeof seg);
                	
                	while(1){/// send package till timeout is achieved
						while(cantidad<recibidos){//sending the msj 20 by 20
							memcpy(seg,socketMessage+cantidad,20);
							msjSize=write(serial_port,seg,strlen(seg));///envia msj
   							cantidad=cantidad+msjSize;
   							usleep(100000);
						}////all the package has been sent

						
						cantidad=0;
						
						r_set = all_set;
						tv.tv_sec =2 ;
						if(select(maxfd, &r_set, NULL, NULL, &tv)==0){///is there an answer? check it out!!
							contador++;
							printf("No answer found\n");
							if(contador==MAX){
							contador=0;
								printf("ERROR. Lost connection\n\n");
								send(descriptor,error,sizeof error,0);
								goto salida;
							}
							continue; //go and try it again
						}

						//if we are here is because the receiver got the package and sent us a response
                        contador = 0;
		

						memset(seg,'\0',sizeof seg);
						memset(buff_mess,'\0',sizeof buff_mess);
						while(cantidad<recibidos){///receiving the package
							memset(seg,'\0',sizeof seg);
							bytes_confirm = read(serial_port, seg, sizeof(seg));
							cantidad=cantidad+bytes_confirm;
							strcat(buff_mess,seg);
						}
						
						printf("\nThe other transceiver resent me the package: <<%s>>\n",buff_mess);
						
						///comparison
	                    int confirmation =strcmp(buff_mess, socketMessage);//comparing the package sent and received
                        if (confirmation==0){
                            write(serial_port, "1", 2); // we send a flag to confirm the status of the package
                            printf("Package is correct\n\n");// tudo bem

                        }
                        else{
                            write(serial_port, "0", 2);// Aqui tenemos que enviar bandera
                            printf("\n\nPackage is incorrect\n\n");
                            reenvios++;
                            if(reenvios == MAX){
                                printf("%d Attempts sending the package.\nLost connection. Closing..\n",MAX);
                                reenvios = 0;
                                goto salida;
                            }        
                            continue; ///GO TO THE WHILE AND TRY IT AGAIN
                        }
                        
                        break;  ///if we break the while is 'cause the package has been sent successfully
				
					}//// while
					
						
					if(socketMessage[2]=='~'){////we have to receive an image from AWS and send it to the raspberry by serial port(throug transceiver) 
						
						send(descriptor,socketMessage,strlen(socketMessage)+1,0);
						memset(socketMessage,'\0',sizeof socketMessage);
						puts("Receiving image from AWS by socket");
						recv(descriptor,socketMessage,sizeof(socketMessage),0);/// here we receive the size of the image in bytes
						printf("Image size: %s\n",socketMessage);///
						int imageSize= atoi(socketMessage);  /// from str to long
						
						FILE* fpx = fopen( "awsPc.bmp", "wb");
						int tot=0;
						int b;
						if(fpx != NULL){
							while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size.
								b=recv(descriptor, socketMessage, 1,0);// receive the bytes, 'b' saves the amount of bytes received
								tot+=b; // counting the amount of bytes receives
								fwrite(socketMessage, 1, b, fpx);  //writing the file
							}
							printf("Image received\nTotal bytes received: %d\n",tot);
							if (b<0)
							   perror("Receiving");
							fclose(fpx);
						} 	//end of writing the image received
						else{
							perror("File");
						}					
					
						memset(message,'\0',sizeof(message));			
						printf("\nResending image through transceiver \n");
						FILE *fp = fopen("awsPc.bmp", "rb");   ///// open the image that we are gonna send!
						if(fp == NULL){
							perror("File");
							//return 2;
						}

		               //sleep(100000);
						
						///we need to send the size of the file 
						
						fseek(fp,0,SEEK_END);///
						long size=ftell(fp); //
						sprintf(message,"%ld",size);///
						usleep(1000000);
						write(serial_port,message,strlen(message));// sending the file's lenght
						fseek(fp,0,SEEK_SET);//
						total=0;
						contador=0;
						lost=0;
						
						
						unsigned char buffer[20];
						//read(serial_port,buffer,sizeof(buffer));
						memset(buffer,'\0',sizeof(buffer));
						memset(buffer,'\0',sizeof(imagen));
						
						while( (n = fread(imagen, 1, sizeof imagen, fp)) > 0 ){ //we read the bytes on the file, 'n'gets the amount of read bytes
														
							while(1){
						//	sleep(1);
								wc=write(serial_port, imagen,n);///send the package
								r_set = all_set;
								tv.tv_sec = 2; //set time out
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
									contador++;
									printf("No answer found\n");
									if(contador==10){
										contador=0;
										printf("ERROR. Lost connection\n\n");
										send(descriptor,error,sizeof error,0);
										fclose(fp);//closing image file
										goto salidaReceiving;
									}
									continue;//answer was not gotten, go, send the package again
								}
								contador=0;
							//sleep(100000);
								//if we are here is because we send the package, the receiver got it and resent it to us :) AWESOME!
								rc=read(serial_port,buffer,n);////read resent package
								if(memcmp(imagen,buffer,n)==0){ ///if they are the same
									write(serial_port,"1",3); ///send flag to confirm success
									break;
								}
								write(serial_port,"N",3);///send to confirm failure
								lost++;
								printf("Incorrect Package. Retrying...\n");
								if(lost==10){
									lost=0;
									fclose(fp);
									send(descriptor,error,sizeof error,0);
									printf("Error. All packages are incorrect\n");
									goto salidaReceiving;					

								}
							}
							total=total+n;
		                    porcentaje = (float)total/size * 100;
		                    printf("Read:%d ,Written: %d ,Received:%d , Total:%d , %3.2f %%\n\n",n,wc,rc, total, porcentaje);
							
						}
						
						printf("\nImagen has been sent successfully.\nTotal bytes: %d\n",total);
						printf("Este proyecto se lo dedico a Ernesto por considerarme el mejor estudiante. Limber la leyenda\n");
						if(fclose(fp)){
							printf("File has been closed unsuccessfully\n");	
						}

					}
   
                    // EXIT PROTOCOL

                    printf("\n####Sending Exit message####\n");
                    usleep(500000);
                    write(serial_port, final, sizeof(final)); // Send exit message
					usleep(500000);
                    
					
    
				}
				else{
					printf("The header is incorrect\nUnsuccessful Startup Protocol\n");
				}
			
			salida:
			printf("-----Connection finished-----\n\n\n\n\n");
			FD_CLR(serial_port, &r_set);///nos olvidamos que algun momento recibimos del serial_port
		}
		
		
		
		
	
        // -------------------------------------------- Receive from transceiver throug serial port ----------------------------
		
		if(FD_ISSET(serial_port, &r_set)){//check to see if we can read from transceiver

			char verify[] = "EL5522TALLERDECOMU";//this is our header

			memset(buff_in, '\0', sizeof(buff_in));
			num_bytes = read(serial_port, &buff_in, sizeof(buff_in));////we are receiving the verified msj 'EL5522TALLERCOMU'

			int counter_verify = strcmp(buff_in, verify);
			if(counter_verify == 0){ // Verify that header is correct
				int cont_reenvios = 0;
		
				printf("\n\n\n\n\n-------Initialization of the protocol-------\n");
				char init[] = "InitCon";
				sleep(1);
				write(serial_port, init, strlen(init)); // send message to know that the connection was initialized
				
				///We send confirmation message and wait for the income message
				///Receiving msj
				
				memset(buff_in, '\0', sizeof(buff_in));
				read(serial_port,buff_in,sizeof(buff_in));//we receive the total size of the message
				int tamanoMSJ=atoi(buff_in);
	
				int cantidad=0;
				char segment[100];
				memset(buff_in, '\0', sizeof(buff_in));
				////leyendo con timeout
correc:
				while(true){///leelo bien
					r_set = all_set;
					tv.tv_sec = 2; 
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
						
						contador++;
						printf("No answer\n");
						if(contador==MAX){
							contador=0;
							printf("No answer. Lost Connection\nClosing...\n\n");
							memset(buff_in, '\0', sizeof(buff_in));
							goto salidaReceiving;///go to exit
						}
						continue;///go to the while and try it again
					}
					else{ ///we have an entrance
						contador = 0; // restart counter
						
						while(cantidad<tamanoMSJ){ //receiving msj 20 by 20
							memset(segment,'\0',sizeof segment);
							num_bytes=read(serial_port,segment,sizeof(segment));//
							cantidad=cantidad+num_bytes;
							
							strcat(buff_in,segment);///concatenate the packages
							
						}
						//here we are done receiving
						cantidad=0;
						
						//printf("Msj:%s  bytes:%d\n\n",buff_in,num_bytes);
						break; ///break the while of receiving packages 20 by 20
					}
				}
				
				////////Now we have to resend it to verify///// 
				
//next to lines is a test. If wanna test it, uncomment them and comment the while after these lines and compile it
//				char buff_temp[6] = "Nuevo"; // Este es para probar
//			    write(serial_port, buff_temp,sizeof(buff_temp)); //prueba para enseñarle a papi que sirve
				
				msjSize=0;
				while(cantidad<tamanoMSJ){///resending the msj 20 by 20
					memcpy(segment,buff_in+cantidad,20);
					msjSize=write(serial_port,segment,strlen(segment));///envia msj
					cantidad=cantidad+msjSize;
					usleep(100000);
				}
				printf("Message has been resent to verify!\n\n");
				
				// Recieving a flag( 0 - incorrect ) another way it is correct
				read(serial_port, buff_aux, sizeof(buff_aux)); //receive the flag

				if (buff_aux[0] == '0'){ // message was incorrect
					
					reenvios++;//sum the amount of attempts
					
					if(reenvios == MAX){ 		
						printf("\nMaximum amount of attempts achieved\n Lost connection.\nClosing...\n\n");
						reenvios = 0;
						goto salidaReceiving;					
					}
					
					printf("\n------Incorrect Package. Trying again---------------\n\n");
					goto correc; // go to correc and try it again
				}

				
				printf("Package is correct <<%s>>\n",buff_in);
				

				if(buff_in[2]=='~'){ // we are receiving an image
					memset(message,'\0',sizeof message);
					printf("Receiving an image \n");
					
					read(serial_port, message,sizeof message);///read the lenght of the file
				//	printf("Image size: %s\n\n",message);//// :)
					imageSize = atol(message);
					printf("Image size: %ld\n\n",imageSize);
					usleep(2000000);
					write(serial_port,'\0',3);
					FILE* fp = fopen( "transPc.bmp", "wb");
					int tot=0;
					int b;
					usleep(10000);
					if(fp != NULL){
						//printf("\n\nGrande palomilla\n\n");
				
						while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size
							while(1){////receiving packages 20 by 20
								r_set = all_set;
								tv.tv_sec = 2; 
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){/////do we have a incoming package?? NO??
									contador++;
									printf("No package\n");
									if(contador==10){
										contador=0;
										printf("Lost connection.\nClosing...\n\n");
										fclose(fp);//closing the file
										goto salidaReceiving;//go to exit
									}
									continue;
								}
								//here we have something to read
								contador=0;
								b=read(serial_port, buff_aux, sizeof(buff_aux));// receive the bytes, 'b' saves the amount of bytes received
								//usleep(100000);
								wc=write(serial_port,buff_aux, b);///resend package
								//usleep(100000);
								r_set = all_set;
								tv.tv_usec = 10000;
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
									continue;
								}




								read(serial_port, bandera, sizeof(bandera));//read a flag to verify the package's integrity
								//usleep(100000);
								if(bandera[0]=='1'){/// the package is correct, break the while and go for the next package

									break;
								}
								///if we are here is because the package was wrong
								lost++;
								printf("Incorrect package. Retrying\n");
								if(lost==20){
									lost=0;
									fclose(fp);
									printf("Error. Lost connection\n");
									goto salidaReceiving;
								}
						
							}
							tot+=b; // counting the amount of bytes receives
							porcentaje = (float)tot/imageSize *100;
							printf("Read %d ,Forwarded:%d ,Total %d , %3.2f %%\n\n",b,wc,tot, porcentaje);
							fwrite(buff_aux, 1, b, fp);  //writing the file
						}
						printf("\n\nImage Received \nTotal Bytes received: %d\n\n",tot);
						if(fclose(fp)!=0){
							printf("Error closing the file\n");
						}
						

						///Now we have to resend the image to the AWS server through sockets
						
////**************************////////////////////////////////////////////////////////**********************
						send(descriptor,buff_in,strlen(buff_in)+1,0); ///sending ~ to AWS
						char miedo[100];
						
						memset(miedo,'\0',sizeof miedo);
						recv(descriptor,miedo,strlen(miedo)+1,0); 
						
						if(miedo[0]=='P'){ //P usted no tiene saldo       q Sent! 
							printf("ERROR. %s",miedo);
							goto salidaReceiving;
						}
						
						
						FILE *fp2 = fopen("transPc.bmp", "rb"); 
						if(fp2 == NULL){
							perror("File");
						}
						int tot=0;
						int n;
						long size=0;
						char tamano[100];
						char imageUsuario[100];
		
						fseek(fp2,0,SEEK_END);
						size=ftell(fp2);
						sprintf(tamano,"%ld",size);
						usleep(1000);//to ensute that aws is gonna receive this
						send(descriptor, tamano, sizeof(tamano) , 0);//sending the image size to the AWS server
						printf("Sending msj to AWS server througk socket\n\n");
						fseek(fp2,0,SEEK_SET);
						
						while( (n = fread(imageUsuario, 1, sizeof(imageUsuario), fp2)) > 0 ){ 
							tot+=n; 
							send(descriptor, imageUsuario, n, 0);
						}
						printf("Image has been sent\nTotal Bytes sent: %d \n",tot);
						
						fclose(fp2);///closing the file
						
					} 	//end of writing the image received
					else{
						perror("File");
					}
				} // cerrando buff_in = ~
				
				else{///if the message was not an image, it is a simple message
					send(descriptor,buff_in,strlen(buff_in)+1,0);
				}
				// Closing of the protol
				printf("\n####Closing Protocol####\n");
				
				contador=0;
				
				memset(buff_in, '\0', sizeof(buff_in));
				while(true){///leelo bien
					r_set = all_set;
					tv.tv_sec = 2; 
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
						
						contador++;
						printf("Waiting for exit message\n");
						if(contador==3){
							contador=0;
							printf("Lost connection.\nClosing...\n\n");
							goto salidaReceiving;
						}
						continue;
					}
					else{
						contador = 0; // para reiniciar contador
						num_bytes=read(serial_port,buff_in,sizeof(buff_in));//
						int confirmation = strcmp(buff_in, "ENDtallerEL5522"); // Comparo con ENDtallerEL5522
						if(confirmation == 0){
							printf("Exit message received successfully:  %s\n", buff_in);
							
						}
						else
							printf("\n\nExit message received unsuccessfully: %s\nClosing....\n", buff_in);
					} // closing the verification						
								
					break;
				}
			salidaReceiving:
			printf("-----Connection has beed finished-----\n\n\n");
			}
		}//cerrando si llego mensaje nuevo
	}//closing while
}
