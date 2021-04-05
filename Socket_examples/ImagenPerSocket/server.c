#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char *argv[]){

    int fd =0, confd = 0,b,tot;
    
    char buff[100];
    int num;
//////////////////////////////////////////////////
//// creacion de sockets    
	struct sockaddr_in serv_addr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket created\n");

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(buff, '0', sizeof(buff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(fd, 10);
///////////////////////////////////////////////////////////


    while(1){
        confd = accept(fd, (struct sockaddr*)NULL, NULL);
        if (confd==-1) {
            perror("Accept");
            continue;
        }
        FILE* fp = fopen( "2.bmp", "wb");
        tot=0;
        if(fp != NULL){
            while( (b = recv(confd, buff, sizeof(buff),0))> 0 ) {
            	printf("Bytes received:%d\n",b);
                tot+=b;
                printf("Bytes total count:%d\n\n",tot);
                fwrite(buff, 1, b, fp);
            }

            printf("Received byte: %d\n",tot);
            if (b<0)
               perror("Receiving");

            fclose(fp);
        } else {
            perror("File");
        }
        close(confd);
    }

    return 0;
}
