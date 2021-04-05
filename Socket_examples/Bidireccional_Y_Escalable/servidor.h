#define maxClientes 4

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



