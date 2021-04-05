#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
int main(){
	int serial_port = open("/dev/ttyUSB0", O_RDWR);  //////en raspberry pi   /dev/ttyS0

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

	// Write to serial port
	printf("Type your message: \n"); 
    char msg[100]; // Mesage string that will be sent
    scanf("%[^\n]%*c", msg); // scanf the message
    
    int bytes_num; // count the number of bytes that need to be sent
    char bytes_size[10];
    bytes_num = strlen(msg);
    sprintf(bytes_size, "%d", bytes_num);
   

    char header[] = "EL5522TALLERDECOMU";
    write(serial_port, header, sizeof(header));//send header to the other endpoint to confirm connection:

    // recieve confirmation of the client
    char buff_mess[100];

    char buff_confirm[8];
    int bytes_confirm = read(serial_port, &buff_confirm, sizeof(buff_confirm));
    printf("\n\nInicio de protocolo, confirmacion de conexion\nBytes recibidos: %i, El mensaje confirmacion es:%s\n", bytes_confirm, buff_confirm);

    char messconfi[8]="InitCon";
    int confirmation = strcmp(messconfi, buff_confirm);
    if(confirmation == 0){

        int reenvios = 0;

        printf("The connection was stablished successfully");

        printf("\n\n###############################################\nThe message that will be sent is: %s \nWith a size of: %d bytes\n##############################################\n\n\n", msg, bytes_num); 

        write(serial_port, bytes_size, strlen(bytes_size)+1);////sending the number of bytes

        read(serial_port, &buff_confirm, sizeof(buff_confirm));///

        memset(&buff_confirm, '\0', sizeof(buff_confirm));

        printf("Sending message: ...\n\n");
        char aux[3];

        for(int u=0; u < bytes_num+2; u++){
            //memset(&buff_confirm, '\0', sizeof(buff_confirm));
            //printf("Sending the byte number %d: %c\n...\n", u, msg[u]);
            //aux[0] = msg[u];

            if(u<2){
                write(serial_port, "M2", 2);
                read(serial_port, &buff_mess, sizeof(buff_mess));
                continue;
            }

            reenvio:

            sprintf(aux, "%d",u-2);
            aux[1] = msg[u-2];
            aux[2] = '\0';

            bytes_confirm = write(serial_port, aux, strlen(aux));
            printf("Los bites que mande: %d, el mensaje: %s\n", bytes_confirm, aux);
            memset(&buff_mess, '\0', sizeof(buff_mess));
            read(serial_port, &buff_mess, sizeof(buff_mess)); // Aqui llega el que compara
            printf("Lo que estoy leyendo: %s\n", buff_mess);
            int bandera = strcmp(buff_mess, aux); // cambiar el hola por aux////////////////////AQUIIIIIIIIIIIIIIII
            char flag[2];
            sprintf(flag, "%d", bandera);
            write(serial_port, flag, sizeof(flag)); // EL de confirmacion siempre
            if(bandera != 0){ // Lo envio mal el mensaje
                printf("\n\n-----------------------------------------Me entro mal... reenviando\n\n");
                reenvios++;
                if (reenvios>20){ // SUPERO LA CANTIDAD MAXIMA DE REENVIOS
                    printf("\n\n\nSe cumplio la cuota maxima de reintentos.... :CCCCCCCCCC\n\n\n");
                    read(serial_port, &buff_mess, sizeof(buff_mess)); // por compromiso
                    goto salir;
                }
                
                read(serial_port, &buff_mess, sizeof(buff_mess)); // Recibir por compromiso para porque tengo que mandar en el siguiente
                goto reenvio;
            }
            else{
                printf("\n\n-----------------------------------------Le llego el mensaje bien al otro\n\n");
            }
            read(serial_port, &buff_mess, sizeof(buff_mess)); // Recibir por compromiso porque tengo que mandar en el siguiente 
            
            
        }
	printf("\n\nYour msj has been sent successfully\n\n");
    
    //send all the message
        //write(serial_port, msg, bytes_num);
    }

salir:

    write(serial_port, "END", 3);
    read(serial_port, &buff_mess, sizeof(buff_mess));
    confirmation = strcmp("END", buff_mess);
    if(confirmation == 0){ // THe end message was received properly
	    close(serial_port);
        printf("\n\n######################END protocol recevied properly########################\n\n");
    }
}
