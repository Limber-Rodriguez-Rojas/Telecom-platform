//message buffer related delcartions/macros
int buffer_message(char * message);
int find_network_newline(char * message, int inbuf);


static int inbuf; // how many bytes are currently in the buffer?
static int room; // how much room left in buffer?
static char *after; // pointer to position after the received characters



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

int find_network_newline(char * message, int bytes_inbuf){
    int i;
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
