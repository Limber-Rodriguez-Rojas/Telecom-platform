#include <stdio.h>
#include <string.h>
#include<stdlib.h>
// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
int main(){
	int serial_port = open("/dev/ttyS0", O_RDWR);

	// Create new termios struc, we call it 'tty' for convention
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

	// Set in/out baud rate to be 9600
	cfsetispeed(&tty, B9600);
	cfsetospeed(&tty, B9600);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	char verify[] = "EL5522TALLERDECOMU";

	char buff_in[100];
	int num_bytes = read(serial_port, &buff_in, sizeof(buff_in));////we are receiving the verified msj 'EL5522TALLERCOMU'
	if (num_bytes < 0) {
		printf("Error reading: %s", strerror(errno));
	}

	int counter_verify = strcmp(buff_in, verify);
	if(counter_verify == 0){ // Verify that header is correct
		
		int cont_reenvios = 0;
		
		printf("Initialization of the protocol..\n");
		char init[] = "InitCon";
		write(serial_port, init, strlen(init)+1); // send message to know that the connection was initialized
		printf("Confirmation msj was sent!\n\n");

		int num_bytes2 = read(serial_port, buff_in, sizeof(buff_in));////we receive the size of the msj
		int numero = atoi(buff_in);
		printf("######################################\nInitializing the reception of the msj\nSize of the msj to be received: %d\n######################################\n\n", numero);//size of the msj
		memset(&buff_in, '\0', sizeof(buff_in));

		write(serial_port, init, strlen(init)+1);////sending 'InitCon' again
	
		char msg_final[100];
		char aux[3];

		for(int o= 0; o<numero+2; o++){
			
			if(o<2){///esto es para leer bit basura, calentando el transceiver
				read(serial_port,&buff_in,2);
				write(serial_port,"M2",2);
				continue;
			}

			reenvio:

			read(serial_port, &buff_in, 2);
			printf("\n\nRecibo:%s\n", buff_in);
			msg_final[o-2] = buff_in[1]; // Store the char of the message
			
			aux[0] = buff_in[0]; // THis is the index
			aux[1] = buff_in[1]; // This is the char of the message
			aux[2] = '\0';
			num_bytes2 = write(serial_port, aux, strlen(aux));
			printf("Respondo,msj: %s #bytes:%d",aux,num_bytes2);
			printf("\nSe guardo en final[%d]= %c\n\n", o-2, msg_final[o-2]);

			read(serial_port, &buff_in, sizeof(buff_in)); // received the FLAG
			if (buff_in[0] != '0'){ // 
				cont_reenvios++;
				if(cont_reenvios>20){
					printf("\n\nSe cumplio la cuota maxima de reenvios :CCCCCCCCCCCCCC\n\n");
					write(serial_port, "!!", 2); // Enviar mensaje por compromiso
					goto salir;
					
				}
				//goto reenvio;
				printf("\n\n-----------------------------Me llego mal... recibiendo de nuevo");
				write(serial_port, "!!", 2); // Escribo por compromiso porque tengo que mandar en el siguiente
				goto reenvio;
			}

			else{
				printf("\n\n-----------------------------Recibi el mensaje bien\n\n");
			}
			write(serial_port, "!!", 2); // Mandar por compromiso porque recibir en el siguiente
		
		}

		msg_final[numero] = '\0';///end of the buffer that saves the final msj
		printf("\nThe message received is:%s  \nTotal number of bytes: %i\n\n\n", msg_final, numero);

	}
	else{
		printf("Not able to stablish connection protocol, the message was: %s\n", buff_in);
	}

salir:

	read(serial_port, &buff_in, sizeof(buff_in));
	counter_verify = strcmp(buff_in, "END");
	if (counter_verify == 0){
		write(serial_port, "END", 3);
		printf("\n\n############################### END protocol sent properly########################3##\n\n");
		close(serial_port);

	}
	
}
