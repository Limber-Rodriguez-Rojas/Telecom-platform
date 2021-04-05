// MariaDB API program
// Needs to be installed the mariadb client
// Use the following command:
// gcc mysql.c -o ejec $(mariadb_config --libs --cflags)
// ./ejec

#include <stdio.h>
#include <mysql.h>

// Pointer to initiatee Mysql
MYSQL *my;
 
int main( void ){

    // Values to stablish connection
    char host[20];
    char user[20];
    char pass[20];
    char db[20];
 
    // Init Mysql before trying to establish remote connection
    my = mysql_init(NULL);
 
    // Add the values to the strings
    sprintf(host,"18.191.92.207");
    sprintf(user,"root");
    sprintf(pass,"proyecto");
    sprintf(db,"clientes");

    if (my == NULL ){ // Did not initialized correctly
        printf("Cant initalisize MySQL\n");
        return 1;
    }
 
    if( mysql_real_connect (my,host,user,pass,db,0,NULL,0)  == NULL) {
        printf("Error cant login\n");
    } else { // Successful remote login in MariaDB
        const char *user;
        mariadb_get_infov(my, MARIADB_CONNECTION_USER, (void *)&user); 
        printf("Login correct with user: %s\n\n", user);

        // Here it starts to modify values to the db where it was initialized (mysql_real_connect)
        int option;
        char query[1000];
        char nombre[1000];
        char saldo[1000];
        while(1){
            printf("\n\n-----------SELECT YOUR OPTION: -------------\n 1. Add a client \n 2. Remove a client \n 3. Edit a client \n 4. Exit\n");
            scanf("%d", &option);
            if(option == 1){ // ADD A CLIENT
                printf("Digite el nombre de la persona que desea agregar: \n");
                scanf("%s", nombre);
                printf("\nDIgite el saldo de la persona: \n");
                scanf("%s", saldo);
                sprintf(query, "INSERT INTO saldos(nombre, saldo) VALUES ('%s', '%s')", nombre, saldo);    
            }
            if(option == 2){ // REMOVE A CLIENT
                printf("\nDigite el nombre de la persona a borrar:\n");
                scanf("%s",nombre);
                sprintf(query, "DELETE FROM saldos WHERE nombre = '%s'", nombre);
            }
            if(option == 3){ // EDIT A CLIENT
                printf("\nDigite el nombre de la persona a cambiar los datos:\n");
                scanf("%s", nombre);
                printf("\nDigite el nuevo saldo de la persona\n");
                scanf("%s", saldo);
                sprintf(query, "UPDATE saldos SET saldo = '%s' WHERE nombre = '%s'", saldo, nombre);
            }
            if(option == 4){
                mysql_close(my);
                printf("\nConnection closed\n");
                return 0;
            }

            // Returns zero when the query results in no errors 
            if(mysql_query(my, query)==0){
                printf("Operation completed\n\n");
            }
            else
                printf("Error performing the operation");
        }
    }
    
    mysql_close(my);
    return 0;
}
