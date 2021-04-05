#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h> 


//directives are above (e.g. #include ...)

//message buffer related delcartions/macros
int buffer_message(char * message);
int find_network_newline(char * message, int inbuf);

#define COMPLETE 0
#define BUF_SIZE 256

static int inbuf; // how many bytes are currently in the buffer?
static int room; // how much room left in buffer?
static char *after; // pointer to position after the received characters
//main starts below


int buffer_message(char * message){

    int bytes_read = read(STDIN_FILENO, after, 256 - inbuf);
//    char auxiliar[1];
//	auxiliar[0]='A';
	
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


int find_network_newline(char * message, int bytes_inbuf){
    int i;
//	char auxiliar[1];
//	auxiliar[0]='A';
//	strcat(message,auxiliar);
    for(i = 0; i<inbuf; i++){
        if( *(message + i) == '\n')
        return i;
    }
    return -1;
}


int get_client_name(int argc, char **argv, char *client_name){
	if(argc>1)
		strcpy(client_name,argv[1]);
	else
		strcpy(client_name,"no name");
	return 0;
}





int main(int argc , char **argv){
    

	char client_name[256];  ///where the name is stored
	get_client_name(argc,argv,client_name);
	printf("Client: '%s'\n",client_name);


	char message[256] , server_reply[256];

	int sock;
    struct sockaddr_in server;

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("172.26.45.186");
    server.sin_family = AF_INET;
    server.sin_port = htons( 9034 );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
        perror("connect failed. Error");
        return 1;
    }
	//insert the code below into main, after you've connected to the server
	puts("Connected");    

	send(sock,client_name,strlen(client_name),0); ///enviar nombre del cliente al server
	puts(client_name);


	//set up variables for select()
	fd_set all_set, r_set;
	int maxfd = sock + 1;
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set); FD_SET(sock, &all_set);
	r_set = all_set;
	struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;

	//set the initial position of after
	after = message;

	
	//keep communicating with server
	for(;;){

		r_set = all_set;
		//check to see if we can read from STDIN or sock
		select(maxfd, &r_set, NULL, NULL, &tv);

		if(FD_ISSET(STDIN_FILENO, &r_set)){

		    if(buffer_message(message) == COMPLETE){
		        //Send some data

				char a[1];
				a[0]=' ';
				strcat(message," FROM:");
				strcat(message,client_name);///al agregar esto se envia el nombre del cliente emisor en el msj
	
					
//				printf("\n\n el mensaje a enviar es: %s\n",message);
		        
				if(send(sock, message, strlen(message) + 1, 0) < 0)//NOTE: we have to do strlen(message) + 1 because we MUST include '\0'
		        {
		            puts("Send failed");
		            return 1;
		        }
				printf("\n");
		        puts(client_name);
		    }
		}

		if(FD_ISSET(sock, &r_set)){
		    //Receive a reply from the server
		    if( recv(sock , server_reply , 256 , 0) < 0)
		    {
		        puts("recv failed");
		        break;
		    }
			
			for (int i=1;server_reply[i]!='\0';i++){
		    	printf("%c", server_reply[i]);
		    }
		    printf("\n");
		    server_reply[0]='\0';

		}
	}

	close(sock);
	return 0;
//end of main
}
