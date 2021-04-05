
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"   // port we're listening on
#define maxClientes 4

int msjNuevo=0;  //cada vez que un cliente se conecta se recibe un msj del mismo diciendo el name


/////se crea una estructura de cliente, se asigna fd(file descriptor) y nombre
struct newC{
	int fd;
	char name[1];
};


////creación de lista de estructura de clientes, cantidad total permitida
struct newC clientes[maxClientes];

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(void){

	for(int a=0;a<maxClientes;a++){
		clientes[a].fd=0;			/////inicialmente la lista de clientes no tiene nada
	}



    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

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
    
 ////// guarda el socket del server en la lista clientes.   
	clientes[0].fd=listener;
	clientes[0].name[0]='S';

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
                    memset(buf, 0, 256);
                    nbytes = recv(i, buf, sizeof buf, 0); ///recibe msj. Está vacio o no?
                    if (nbytes <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } 
						else {
                            perror("recv");
                        }
						clientes[i].fd=0;//vuelvo a limpiar lista clientes
//						
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } 

					else {
                        // we got some data from a client
                        
						if(msjNuevo){
							msjNuevo=0;
							printf("Nuevo cliente:%s, con fd:%d\n",buf,i);

							for(int a=0;a<maxClientes;a++){
								if(clientes[a].fd==0){
									clientes[a].fd=newfd; //agrega el nuevo cliente a la lista
									clientes[a].name[0]=buf[0];
									break;
								}
							}

						}
						/*for(int y=0;y<maxClientes;y++){
							printf("\n\nnombre:%s con fd:%d\n",clientes[y].name,clientes[y].fd);
						}*/
						////revisar para quien va el msj
						else{
							for(int x=0;x<maxClientes;x++){
								if(clientes[x].name[0]==buf[0]){
									printf("Encontre cliente destino\n");		
									send(clientes[x].fd,buf,nbytes,0);
								}	
							}	
						}
/*								
						for(j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }*/
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}
