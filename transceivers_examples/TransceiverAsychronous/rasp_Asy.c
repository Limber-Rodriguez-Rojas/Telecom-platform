#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <pthread.h>
#include <sys/select.h>
#include <arpa/inet.h> //inet_addr

#define COMPLETE 0
#define MAX 3

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
char message[256] ;
unsigned char imagen[20];

int buffer_message(char * message);
char buff_in[100];
unsigned char buff_aux[21];
char bandera [3];
float porcentaje;
bool calentando=true;


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
	serial_port = open("/dev/ttyUSB1", O_RDWR);  //////en raspberry pi   /dev/ttyS0
	maxfd=serial_port+1;
	
	//////preparing select()
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set);
	FD_SET(serial_port, &all_set);
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
        tv.tv_sec = 10;
		select(maxfd, &r_set, NULL, NULL, &tv);
		
		if(FD_ISSET(STDIN_FILENO, &r_set)){
			if(buffer_message(message) == COMPLETE){
            
				char header[] = "EL5522TALLERDECOMU";
                char final[] = "ENDtallerEL5522";
                char messconfi[8]="InitCon";
                char buff_mess[100];
                char buff_confirm[8];				
              //  write(serial_port, header, sizeof(header));//send header to the other endpoint to confirm connection:


            	while(1){///enviar msj de inicio de protocolo hasta que se acabe timeout
			        msjSize=write(serial_port,header,strlen(header));/////send header to the other endpoint to confirm connection:
					printf("Escribi msj: %s \n",header);
					r_set = all_set;
					tv.tv_sec = 5; 
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){///timeout de 3 veces
						contador++;
						if(contador==MAX){
						contador=0;
							printf("ERROR. El receptor es inalcanzable\n\n");
							goto salida;
						}
						continue;
					}
                    contador = 0;
                    break;
			
				}//// salliendo del while

				
                // recieve confirmation of the client

               	memset(buff_confirm,'\0',sizeof buff_confirm);
                int bytes_confirm = read(serial_port, &buff_confirm, sizeof(buff_confirm));
                printf("\n\nInicio de protocolo\nEl mensaje confirmacion es:%s\n", buff_confirm);

                
                int confirmation = strcmp(messconfi, buff_confirm);		
				
                if(confirmation == 0){
                    printf("Protocolo de inicio completado exitosamente\n\n");
					sleep(1);
					
                	// Continue sending the mes;g
                	tv.tv_sec = 3;
                	while(1){///enviar msj hasta que se acabe timeout
				        msjSize=write(serial_port,message,strlen(message));///envia msj
						printf("Escribi msj: %s ---> Tamano %d\n\n",message,msjSize);
						r_set = all_set;
						tv.tv_sec =3 ;
						if(select(maxfd, &r_set, NULL, NULL, &tv)==0){///timeout de 3 veces
							contador++;
							if(contador==MAX){
							contador=0;
								printf("Receptor inalcanzable\n\n");
								goto salida;
							}
							continue;
						}

                        contador = 0;
			
						memset(buff_mess,'\0',sizeof buff_mess);
						bytes_confirm = read(serial_port, &buff_mess, sizeof(buff_mess));//recibir el msj desde la raspb, es el mismo
						printf("Me reenviaron el msj: <<%s>>\n\n",buff_mess);
						
						
						///comparacion
	                    int confirmation =strcmp(buff_mess, message);
                        if (confirmation==0){
                            write(serial_port, "1", 2); // Aqui tenemos que enviar bandera
                            printf("Msj enviado correctamente\n\n");// tudo bem

                        }
                        else{
                            write(serial_port, "0", 2);// Aqui tenemos que enviar bandera
                            printf("\n\nEnvio bandera 0 por error en el msj\n\n");
                            reenvios++;
                            if(reenvios == MAX){
                                printf("\nSe cumplio la cuota maxima de reenvios\n\n");
                                reenvios = 0;
                                goto salida;
                            }        
                            continue;
                        }
                        
                        break;
				
					}//// salliendo del while
					
						
					if(message[0]=='~'){
						memset(message,'\0',sizeof(message)); //se limpia la memoria del msj
						
						printf("Enviaremos imagen \n");
						
						
						FILE *fp = fopen("pequenna.png", "rb");   ///// open the image that we are gonna send!
						if(fp == NULL){
							perror("File");
							//return 2;
						}

		               //sleep(100000);
						
						///we need to send the size of the file 
						int size=0;
						fseek(fp,0,SEEK_END);///esto es para informar el tamaño total del file, puntero se ubica al final
						size=ftell(fp); //se guarda posicion del puntero
						sprintf(message,"%ld",size);///de long a str, se guarda en el buffer a enviar
						usleep(1000000);
						write(serial_port,message,strlen(message));// sending the file's lenght
						fseek(fp,0,SEEK_SET);//se vuelve a colocar el puntero al inicio del file
						total=0;
						contador=0;
						
						
						read(serial_port,buff_in,sizeof(buff_in));
						unsigned char buffer[20];
						memset(buffer,'\0',sizeof(buffer));
						memset(buffer,'\0',sizeof(imagen));
						
						while( (n = fread(imagen, 1, sizeof imagen, fp)) > 0 ){ //we read the bytes on the file, 'n'gets the amount of read bytes
														
							while(1){
							//sleep(100000);
								wc=write(serial_port, imagen,n);
								r_set = all_set;
								tv.tv_sec = 2; 
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
								
									contador++;
									printf("No me llego nada en 2 seg\n");
									if(contador==10){
										contador=0;
										printf("En 20 segundos no recibi nada\n\n");
										fclose(fp);
										goto salidaReceiving;
									}
									continue;
								}
								contador=0;
							//sleep(100000);
								
								rc=read(serial_port,buffer,n);////lee el msj de regreso	
								
								
								/*intf("\n\n");
								for(int po=0;po<n;po++){
									printf("%02X ",imagen[po]);
								}	
								printf("\n");
								for(int pa=0;pa<n;pa++){
									printf("%02X ",buffer[pa]);	
								}
								printf("\n");*/
							//sleep(100000);
				
								if(memcmp(imagen,buffer,n)==0){
									
									write(serial_port,"1",3);
									break;
								}
								write(serial_port,"N",3);
								
							
								
							}
							total=total+n;
		                    porcentaje = (float)total/size * 100;
		                    printf("n:%d ,wc: %d ,rc:%d , total:%d , %3.2f %%\n\n",n,wc,rc, total, porcentaje);
							//printf("\nBytes leido %d, bytes escritos %d, total %d\n\n", n,wc,total);
						}
						
						printf("\nImagen enviada exitosamente. Bytes total %d\n",total);
						printf("Este proyecto se lo dedico a Ernesto por considerarme el mejor estudiante");
						if(fclose(fp)){
							printf("El archivo se ha cerrado incorrectamente\n");	
						}

					}
   
                    // Protocolo de cierre

                    printf("\n####Mandando mensaje de cierre####\n\n");
                    usleep(500000);
                    write(serial_port, final, sizeof(final)); // Manda el mensaje para protocolo de cierre
					usleep(500000);
                    printf("\n\n\nSe cerro correctamente la conexion\n");
                    printf("------------------------------------------------------------------------------------\n\n\n");

                    // FIn de protocolo de cierre
    
				}
				else{
					printf("EL msj de confirmacion se ha recibido erroneamente");
				}
			}
			salida:
			FD_CLR(serial_port, &r_set);///nos olvidamos que algun momento recibimos del serial_port
		}
		
        // -------------------------------------------- Receive part of the program ----------------------------
		
		if(FD_ISSET(serial_port, &r_set)){//check to see if we can read from transceiver

			char verify[] = "EL5522TALLERDECOMU";

			memset(buff_in, '\0', sizeof(buff_in));
			num_bytes = read(serial_port, &buff_in, sizeof(buff_in));////we are receiving the verified msj 'EL5522TALLERCOMU'

			int counter_verify = strcmp(buff_in, verify);
			if(counter_verify == 0){ // Verify that header is correct
		
				int cont_reenvios = 0;
		
				printf("Initialization of the protocol..\n");
				char init[] = "InitCon";
				write(serial_port, init, strlen(init)+1); // send message to know that the connection was initialized
				printf("Confirmation msj was sent!\n\n");
				///envio msj de confirmación y entro en estado de recibir msj
				// Aquiiiiiii recive el mensaje ///////////////////////

				memset(buff_in, '\0', sizeof(buff_in));
				tv.tv_sec = 5; 
				////leyendo con timeout
correc:
				while(true){///leelo bien
					r_set = all_set;
					tv.tv_sec = 5; 
					if(select(maxfd, &r_set, NULL, NULL, &tv)==0){
						
						contador++;
						printf("No me llego nada en 5 seg\n");
						if(contador==MAX){
							contador=0;
							printf("En 15 segundos no recibi nada\n\n");
							goto salidaReceiving;
						}
						continue;
					}
					else{
						contador = 0; // para reiniciar contador
						num_bytes=read(serial_port,buff_in,sizeof(buff_in));//
						printf("Msj:%s  bytes:%d\n\n",buff_in,num_bytes);
						break;
					}
				}
				
				////////reenviarlo///// 
				
				char buff_temp[6] = "Nuevo"; // Este es para probar
//			    write(serial_port, buff_temp,sizeof(buff_temp)); //prueba para enseñarle a papi que sirve
				write(serial_port, buff_in,sizeof(buff_in));
		
				printf("Reenvio msj \n\n");
				
				// Recibiendo la bandera
				read(serial_port, buff_aux, sizeof(buff_aux));
				
				printf("\n\nMe llego la bandera : %s\n\n", buff_aux);

				if (buff_aux[0] == '0'){ // Recibi mal
					
					reenvios++; // Sumo a la cantidad de reenvios
					
					if(reenvios == MAX){ 		
						printf("\nYa cumplio la cuota maxima\n\n");
						reenvios = 0;
						goto salidaReceiving;					
					}
					
					printf("\n------Recibi mal, me regreso al while---------------\n\n");
					goto correc; // Me devuelvo a recibir el mensaje
				}

				
				printf("\nRecibi bien el mensaje:    %s\n\n",buff_in);
				

				if(buff_in[0]=='~'){ // detectar que quiere quiere enviar imag
					memset(buff_in,'\0',sizeof buff_in);
					printf("Recibimos Imagen \n");
					usleep(100000);
					read(serial_port, buff_in,sizeof buff_in);///read the lenght of the file
					printf("Tamano de la imagen %s\n\n",buff_in);//// :)
					imageSize = atol(buff_in);
					write(serial_port,'\0',2);
					FILE* fp = fopen( "2.png", "wb");
					int tot=0;
					int b;
					usleep(10000);
					if(fp != NULL){
						//printf("\n\nGrande palomilla\n\n");
				
						while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size
							
							
							while(1){
								r_set = all_set;
								tv.tv_sec = 2; 
								if(select(maxfd, &r_set, NULL, NULL, &tv)==0){

									contador++;
									printf("No me llego nada en 2 seg\n");
									if(contador==10){
										contador=0;
										printf("En 20 segundos no recibi nada\n\n");
										fclose(fp);
										goto salidaReceiving;
									}
									continue;
								}
								contador=0;
								b=read(serial_port, buff_aux, sizeof(buff_aux));// receive the bytes, 'b' saves the amount of bytes received
								//usleep(100000);
								wc=write(serial_port, buff_aux, b);///reenvio paquete
								//usleep(100000);
								read(serial_port, bandera, sizeof(bandera));//recibo bandera
								//usleep(100000);
								
								
								if(bandera[0]=='1'){

									break;
								}						
							}
					
							tot+=b; // counting the amount of bytes receives
							porcentaje = (float)tot/imageSize *100;
							printf("Leidos %d ,wc:%d ,  total %d , %3.2f %%\n\n",b,wc,tot, porcentaje);
							fwrite(buff_aux, 1, b, fp);  //writing the file
						
						}
						printf("\n\nYa termine de recibir \nBytes recibidos: %d\n",tot);
				    
						if(fclose(fp)!=0){
							printf("Hubo un problema\n");
					}
						printf("Cerre la imagen");
					} 	//end of writing the image received
					else{
						perror("File");
					}
				} // ceerando buff_in = ~
			

				// Closing of the protol
				//
				memset(buff_in, '\0', sizeof(buff_in));
				read(serial_port, buff_in, sizeof(buff_in)); // Recibo ENDtallerEL5522

				int confirmation = strcmp(buff_in, "ENDtallerEL5522"); // Comparo con ENDtallerEL5522

				

				if(confirmation == 0){
					printf("\n\nRecibi correctamente el mensaje de cierre %s\n\nClosing connection\n", buff_in);
					printf("------------------------------------------------------------------------------------\n\n\n");
				}
				else
					printf("\n\nNo recibi correcta mente el mensaje de cierre, %s\n\n", buff_in);

				
				} // closing the verification
				salidaReceiving:
				printf("Coneccion cerrada\n\n");
			}//cerrando si llego mensaje nuevo


	}//closing while
}





