#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

MYSQL *my;

////en este codigo, el server recibe msj enviados por el cliente.
//
///// gcc -o server server.c

int main(int argc , char *argv[]){
	
	// MYSQL initialization
	//
	

	// Declare the variables for the connection 
	char host[20];
	char user[20];
	char pass[20];
	char db[20];

	my = mysql_init(NULL);

	sprintf(host, "localhost");
	sprintf(user, "root");
	sprintf(pass, "proyecto");
	sprintf(db, "clientes");

	if(my == NULL){
		printf("Did not initialized mysql");
		return 1;
	}

	if(mysql_real_connect(my,host,user,pass,db,0,NULL,0)==NULL){
		printf("Error, cannot loggin");
		return 1;
	}

	// End of creating the connection to the mariadb server

	int newfd,nbytes;
	int destinatario;
	char buf[100];
	int descriptor=socket(AF_INET , SOCK_STREAM , 0);//zero means TCP protocol
	if (descriptor== -1){
		printf("Could not create socket");
	}
	puts("Socket created");
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("172.31.14.61"); //inet_addr("ip numbers and dots")
	server.sin_port = htons( 2010  );//server port
	if(bind(descriptor,(struct sockaddr*)&server,sizeof(server))<0){
		perror("Error binding");
		return 1;
	}
	puts("Bind Done!\n");
	listen(descriptor,4); ///specify the maximum number of connections COVID-19

	fd_set master,read_fds; 
	FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(descriptor,&master);  
    
  	int fdmax;
	fdmax=descriptor;
	
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	
	while(1){
		read_fds = master;
		if(select(fdmax+1,&read_fds,NULL,NULL,NULL)==-1){
			perror("select");
			exit(4);
		}
		
		for(int t=0;t<=fdmax;t++){
			if(FD_ISSET(t,&read_fds)){///looking for the descriptor who is asking for a request
				if(t==descriptor){//we got a new connection
					addrlen = sizeof remoteaddr;
					newfd = accept(descriptor, (struct sockaddr *)&remoteaddr, &addrlen);  ////asigna descriptor!
					if (newfd == -1) {
                    	perror("accept");
                    }
                    else{
                    	FD_SET(newfd,&master);
                    	if (newfd > fdmax) {    // keep track of the max
                        	fdmax = newfd;
                        }
                        printf("\nNew connection on socket %d\n",newfd);
                    }  
				}
				
				else{///getting a request from another socket 
					memset(buf,'\0',sizeof buf);
					nbytes = recv(t, buf, sizeof buf, 0);
				aviso:
					switch(t){////asigning the emisor to the message
						case 6:
							buf[1]='P';
							break;
						case 7:
							buf[1]='A';
							break;
						case 8:
							buf[1]='B';		
							break;						
					}
					printf("Mensaje antes de entrar %s\n\n\n\n\n",buf); 					
					if(nbytes<=0){///socket has disconnected
						if(nbytes==0){//connection closed
							printf("\nSocket %d hung up\n",t);
						}
						else{
							perror("recv");
						}
						close(t); // bye!
                        FD_CLR(t, &master); // remove from master set
					}
					
					else{
						
						// -------------------------FIRST make the query to the DB to make sure you have money
					
						if(t==6){
							if (mysql_query(my, "SELECT saldo FROM saldos WHERE nombre='P'")){
								printf("\n\nAN error occurred while trying to make the query to P\n");
							}
	
							MYSQL_RES *result = mysql_store_result(my);

							if (result == NULL){
								printf("\n\nAn error occurred while storing the result\n");
							}

							int num_fields = mysql_num_fields(result);

							MYSQL_ROW row;

							int plata;

							while((row = mysql_fetch_row(result))){

								plata = atoi(row[0]);

								printf("\nI have:  %d and will send a messg with lenght:  %d\n", plata, nbytes);

								if(buf[0] == '1'){ // This is for updating the money

									plata += 1000;
									char query[30];
									sprintf(query, "UPDATE saldos SET saldo = %d WHERE nombre = 'P'", plata);
									mysql_query(my, query);
									memset(buf, '\0', sizeof buf );
									sprintf(buf, "P Se actualizo el saldo correctamente, saldo actual: %d", plata);

								}

								else if(buf[0] == '?'){
									sprintf(buf, "P tiene un saldo de: %d", plata);
								}

						        else if(buf[2]== '^'){
                                     printf("No connection between transceivers\n\n");
                                     
									 if(buf[0]=='A'){
										memset(buf, '\0', sizeof buf);
                                     	strcpy(buf, "A Error while connecting.\n");
										}
                                	 if(buf[0]=='B'){
										memset(buf, '\0', sizeof buf);
									    strcpy(buf, "B Error while connecting.\n");
									}
								}




								else{

								if((plata-nbytes)<0){
									
									printf("No tiene suficiente saldo");
									
									memset(buf, '\0', sizeof buf);
									strcpy(buf, "P Saldo insuficiente\n");
									usleep(6000);						
								}
								
								else{

									printf("\nSe va a proceder a descontar al saldo\n");
									char query[30];
									sprintf(query, "UPDATE saldos SET saldo = %d WHERE nombre = 'P'", (plata-nbytes+3));
									mysql_query(my, query);
								}

								}

								//printf("\nAhora me queda un saldo de: %d \n", plata);								

							}

							mysql_free_result(result);
	
						}

						if(t==7){

							if (mysql_query(my, "SELECT saldo FROM saldos WHERE nombre='A'")){
								printf("\n\nAn error occurred while trying to make the query to A\n");
							}

							MYSQL_RES *result = mysql_store_result(my);

							if(result == NULL){
								printf("\n\nAn error occurred while trying storing the result of A\n");
							}

							int num_fields = mysql_num_fields(result);

							MYSQL_ROW row;

							int plata;

							while((row=mysql_fetch_row(result))){

								plata = atoi(row[0]);

								printf("\nTengo un saldo de %d y envio un mensaje de %d\n", plata, nbytes);

								//plata = atoi(row[0]);
								
								if (buf[0] == '1'){

									plata += 1000;
									char query[30];
									sprintf(query, "UPDATE saldos SET saldo = %d WHERE nombre = 'A'", plata);
									mysql_query(my, query);
									memset(buf, '\0', sizeof buf);
									sprintf(buf, "A Se acutalizo el saldo correctamente, saldo actual: %d", plata);
	
								}

								else if(buf[0] == '?'){
									sprintf(buf, "A tiene un saldo de: %d", plata);
								}

								else{

								if((plata-nbytes)<0){
									printf("\nNO tiene suficiente saldo\n");

									memset(buf, '\0', sizeof buf);
									strcpy(buf, "A NO tiene suficiente saldo\n");
								}

								else{

									printf("\nSe va a proceder a descontar el saldo\n");
									char query[30];
									sprintf(query, "UPDATE saldos SET saldo = %d WHERE nombre = 'A'", (plata-nbytes+3));
									mysql_query(my, query);
								}
								}

							}

							mysql_free_result(result);
						}

						if(t==8){

							if(mysql_query(my, "SELECT saldo FROM saldos WHERE nombre='B'")){
								printf("\n\nAn error occurredd while trying to make the query to B\n");
							}		

							MYSQL_RES *result = mysql_store_result(my);

							if(result == NULL){
								printf("\n\nAn error occurred while sotring the result of B\n");
							}

							int num_fields = mysql_num_fields(result);

							MYSQL_ROW row;

							int plata;

							while((row=mysql_fetch_row(result))){
								
								plata = atoi(row[0]);

								printf("\nTengo un saldo de %d y envio un mensaje de %d\n", plata, nbytes);
	
								//plata = atoi(row[0]);

								if (buf[0] == '1'){
									
									plata += 1000;
									char query[30];
									sprintf(query, "UPDATE saldos SET saldo = %d WHERE nombre = 'B'", plata);
									mysql_query(my ,query);
									memset(buf, '\0', sizeof buf);
									sprintf(buf, "B Se actualizo el saldo correctamente, saldo actual: %d", plata);
								}

								else if(buf[0] == '?'){
									sprintf(buf, "B tiene un saldo de: %d", plata);
								}

								else{

								if((plata-nbytes)<0){
									printf("\nNO tiene suficiente saldo\n");

									memset(buf, '\0', sizeof buf);
									strcpy(buf, "B NO tiene suficiente saldo\n");

								}

								else{
									printf("\nSe va a proceder a descontar el saldo\n");

									char query[30];
									sprintf(query, "UPDATE saldos SET saldo = %d WHERE nombre = 'B'", (plata-nbytes+3));
									mysql_query(my, query);
								}
								}
							}

							mysql_free_result(result);

						}
						
						// --------------------------------- END of making the query to the Database MariaDB

						printf("\n\n---%s---, from %d\n\n",buf,t);
						switch(buf[0]){
							case 'P':
								destinatario=6;
								break;
							case 'A':
								destinatario=7;
								break;
							case 'B':
								destinatario=8;		
								break;						
						}
						
						
						
						if(buf[2]=='~'){
							char message[100];
							char tamano [100];
							
							///////recibiendo imagen
							recv(t,message,sizeof(message),0);/// here we receive the size of the image in bytes
							printf("Tamano de la imagen por recibir: %s\n",message);///si se recibe el tamano de la imagen
							long imageSize= atol(message);  /// from str to long
							FILE* fp;
							if(t==6){
								fp = fopen( "pcAws.png", "wb");
							}
							else{
								fp = fopen( "userAws.png", "wb");
							}
							int tot=0;
							int b=0;
							if(fp != NULL){
								while(tot<imageSize) { /// it is going to receive the amount of bytes of the image's size.
									b=recv(t, message, 1,0);// receive the bytes, 'b' saves the amount of bytes received
									tot+=b; // counting the amount of bytes receives
									fwrite(message, 1, b, fp);  //writing the file
								}
								printf("Ya termine de recibir \nBytes recibidos: %d\n",tot);		
							} 	//end of writing the image received
							else{
								perror("File");
							}
							fclose(fp);	
							
							/////enviando imagen a socket destino
							send(destinatario,buf,strlen(buf),0);///enviamos ~
							recv(destinatario,buf,sizeof buf,0); ////recibimos de regreso ~   /AA^ //BB^
							if(buf[2]=='^'){
								t=6;
								printf("Error connection between transceivers\n\n");
								if(buf[0]=='A')
									destinatario=7;
								if(buf[0]=='B')
									destinatario=8;

								goto aviso;
								
							}
							printf("\nEnviaremos imagen a cliente final \n");
							tot=0;
							b=0;
							FILE* fp2;
							if(t==6){
								fp2 = fopen("pcAws.png", "rb");
							}
							else{
								fp2 = fopen("userAws.png", "rb");
							}
							
			
							if(fp2 == NULL){
								perror("File");
							}			
											
							fseek(fp2,0,SEEK_END);
							long size=ftell(fp2);
							sprintf(tamano,"%ld",size);	
							send(destinatario, tamano, sizeof(tamano) , 0);// se envia el tamaño
							printf("Envie tamano de la imagen\n\n");
							fseek(fp2,0,SEEK_SET);		
							while( (b = fread(message, 1, sizeof(message), fp2)) > 0 ){ 
								tot+=b; 
								send(destinatario, message, b, 0);
							}			
							printf("La cantidad total de bytes enviados es: %d \n",tot);
							printf("Termine de enviar la imagen\n");
							fclose(fp2);					
								
						}
						else{
					
							send(destinatario,buf,strlen(buf)+1,0); ///enviamos msj////envio a pc
							
						}
							
					}
					}

			}//clonsing if(FD_ISSET(t,&read_fds))
	
		}///closing the for()
	}



	return 0;
}
