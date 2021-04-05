
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include </home/an/proyectotallercomu/completo/servidor.h>


#define PORT "9034"   // port we're listening on


int msjNuevo=0;  //cada vez que un cliente se conecta se recibe un msj del mismo diciendo el name
int b,tot;
char buff[1];
char destinatario[1];
int destinatarioFD;
char aux[2];
long imageSize;

int main(void){

	for(int a=0;a<maxClientes;a++){
		clientes[a].fd='\0';			/////inicialmente la lista de clientes no tiene nada
	}
////////////////parte de los sockets

    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
	char bufname[3];///para enviar nombres
    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
///////////////////////////////////////////////////////////////////////////////////
/////escucha y acepta nuevos sockets
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }
    
/////////////////////////////////////////////////////////////////


    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, maxClientes) == -1) {
        perror("listen");
        exit(3);
    }
    
    
	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
 	//limpieza de los files descriptors
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);  //

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop 
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
        
        
            if (FD_ISSET(i, &read_fds)) { //revisa constantemente si hay algo nuevo en el descriptor de lectura, si hay algo entonces revisa
            
            /////////// we got a new one!! /////////////////////////////
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);  ////asigna descriptor!
					
                    if (newfd == -1) {
                        perror("accept");
                    } 
					else {
						msjNuevo=1;
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("\nselectserver: new connection from %s on socket %d\n",
                            inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),  remoteIP, INET6_ADDRSTRLEN),  newfd);
                    }
                } 
		///////////////////////////////////////////////////////////////////

				else {
                    // handle data from a client
                    memset(buf, '\0', sizeof(buf));
                    nbytes = recv(i, buf, sizeof(buf), 0); ///recibe msj. Está vacio o no?
       
                    if (nbytes <= 0) {  // got error or connection closed by client
                      
                        if (nbytes == 0) { // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } 
						else {
                            perror("recv");
                        }
                     	
						/////busca en la lista clientes el fd del cliente que se desconectó para limpiar
						for (int l=0;l<(fdmax-3);l++){
							if(clientes[l].fd==i){//se le informa a todos los clientes que se desconectó
								memset(aux,'\0',sizeof(aux));
								
								aux[0]='X';
								aux[1]=clientes[l].name[0];
								for(int r=0;r<(fdmax-3);r++){
									if(clientes[r].fd!=i){
										send(clientes[r].fd,aux,sizeof(aux),0);
										}
								}

								clientes[l].fd=='\0';
								clientes[l].name[0]='\0';
								printf("Cliente %d ha sido eliminado del registro\n",i);
							}
						}			
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } 

					else {
                        // we got some data from a client
                        
						if(msjNuevo){
							msjNuevo=0;
							printf("Nuevo cliente:%s, con fd:%d\n",buf,i);
					//////////////////////////////////////////////////////////////////////////////////////////////////////
					//////// se agrega el nuevo cliente al registro de clientes.
							for(int a=0;a<(fdmax-3);a++){//es menos 3 pq empieza a asignar fd a partir de 4, 0123 son para std input and output
								if(clientes[a].fd=='\0'){
									clientes[a].fd=newfd; //agrega el id del nuevo cliente a la lista
									clientes[a].name[0]=buf[0]; //agrega el nombre del nuevo cliente a la lista
									break;
								}
							}
					/////////cuando el nuevo cliente se conecta, se hace un broadcasting indicando que se conectó ese cliente!
							for (int l=0;l<(fdmax-3);l++){
								if(clientes[l].name[0]!=buf[0]){
									memset(bufname,'\0',sizeof(bufname));
									bufname[0]='1';
									bufname[1]=buf[0];
									send(clientes[l].fd,bufname,sizeof(bufname),0);
								}
							}
					/////////al nuevo cliente se le envia quienes son los que se conectaron previamente		
							int p;
							for(p=0;p<(fdmax-4);p++){//
									memset(buf,'\0',sizeof(buf));
									buf[0]='1';
									buf[1]=clientes[p].name[0];
									send(i,buf,sizeof(buf),0);	
							}							
						}
					
						////revisar para quien va el msj
						else{
							destinatario[0]=buf[0];
							if(buf[1]!='?'){ /////server reenviando un msj normal, ? es el indicador de que se envia una imagen
								for(int x=0;x<=(fdmax-3);x++){
									if(clientes[x].name[0]==destinatario[0]){
										//printf("Encontre cliente destino\n");		
										send(clientes[x].fd,buf,nbytes,0);
									}	
								}
								memset(buf,'\0',sizeof(buf));
							}
							
							else{/////se envia una imagen
								FILE* fp = fopen( "1.bmp", "wb"); ///file donde se guarda la imagen		
								printf("copiando imagen\n");
								recv(i, buf, sizeof buf, 0);////recibir el tamaño de la imagen en str
								
								for(int x=0;x<=(fdmax-3);x++){
									if(clientes[x].name[0]==destinatario[0]){
										/////le informamos al cliente destino que recibira una imagen y ademas el tamaño de la imagen
										destinatarioFD=clientes[x].fd;
										aux[0]=destinatario[0];
										aux[1]='?';		
										send(destinatarioFD,aux,sizeof aux,0); ///envia '?´ 
										send(destinatarioFD,buf,sizeof buf,0); //envia tamano de imagen al cliente destino
									}	
								}

								imageSize=atol(buf); ///obtiene long del tamano de la imagen
								printf("Image size:%ld\n",imageSize);

								tot=0;
								if(fp != NULL){
									while(tot<imageSize) {
										b = recv(i, buff, 1,0);
										send(destinatarioFD,buff,1,0);
										tot+=b; //printf("Bytes total count:%d\n\n",tot);
										fwrite(buff, 1, b, fp);
									}
									printf("Received byte: %d\n",tot);
									if (b<0)
									   perror("Receiving");

									fclose(fp);
								} else {
									perror("File");
								}
								
								memset(aux,'\0',sizeof(aux));
								memset(buf,'\0',sizeof(buf));
							}	
						}

                    }//END when server receives one msj
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}



