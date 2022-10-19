#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<pthread.h>
#include<sys/wait.h>
#include<stdbool.h>
#define FILE_NAME "contenedores.txt"  // Nombre por defecto del archivo que utilizamos para almacenar el contenedor y la accion

char *finalCommand[100];  // Variable global que va a almacenar el comando que digita el usuario para que sea usado por los hilos
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //se creal el mutex
static FILE *fichero;  // Se crea un fichero

void cadenaTokens(char commands[], char *salida[]){  // Funcion que se encarga de separar el comando por espacios
    char *puntero;
    int i = 0;
    puntero = strtok(commands, " ");  // Se separa el comando por espacios

    while(puntero != NULL){
        salida[i] = puntero;  // Se almacena el token en el nuevo arreglo
        puntero = strtok(NULL, " ");  // Se recorre todo el comando
        i++;  // Se aumenta la posicion
    }
	salida[i] = NULL;
}

void *createContainer(void *params){  // Funcion del hilo que crea los contenedores
	char *args[] = {"sudo", "docker", "run", "-di", "--name", finalCommand[1], finalCommand[2], NULL};  // Una lista con un comando default para crear contenedores
    char buffer[100];         // Aquí vamos a ir almacenando cada línea del txt
	char *listContenedores[100];  // tokens de cada linea
	bool existeContenedor = false; //variable que dira si el contendor existe o no en el txt
	pthread_mutex_lock(&mutex);//se utiliza el bloqueo del mutex
	FILE *fichero = fopen(FILE_NAME, "r"); // Modo lectura
    while (fgets(buffer, 100, fichero)){
        strtok(buffer, "\n");//el buffer tendra linea por linea del txt
		cadenaTokens(buffer, listContenedores); // cada linea que leer buffer se separa en tokens en listcontenedores
		if(strcmp(listContenedores[1], finalCommand[1]) == 0){// compara si el nombre del contenedor a crear ya existe
			existeContenedor = true;//si el nombre dek contenedor esta en el txt es que existe
			break;
		}
    }
	fclose(fichero);//cierra el fichero
	pthread_mutex_unlock(&mutex);//desbloquea el mutex
	if (existeContenedor==false){//si el contenedor no existe
		pthread_mutex_lock(&mutex);//bloqueo con el mutex
		fichero = fopen(FILE_NAME, "a"); 
		fprintf(fichero,"corriendo %s %s\n",finalCommand[1],finalCommand[2]);//se agrega el nuevo contenedor
		fclose(fichero);  // Se cierra el txt o fichero
		pthread_mutex_unlock(&mutex); //desbloqueo del mutex
		pid_t pid;
		pid = fork();  // Se crea un proceso para poder ejecutar el execvp
		if(pid == 0){ // Proceso hijo
			if(execvp(args[0], args) < 0){  // Se ejecuta el execvp con los argumentos pasados
				puts("Error.");
			}
		}
		else{
			wait(NULL);  // El padre espera al hijo
		}
	}else{
		puts("Ese contenedor ya existe");//ese nombre de contenedor ya esta asignado o es decir que ya existe
	}
}

void *listContainer(void *params){  // Funcion del hilo que lista los contenedores
	char *args[] = {"sudo", "docker", "ps", "-a", NULL}; // Una lista con un comando default para listar los contenedores.
	char buffer[100];         // Aquí vamos a ir almacenando cada línea del txt
	char *listContenedores[100];  // tokens de cada linea
	pthread_mutex_lock(&mutex);//bloqueo del mutex
	FILE *fichero = fopen(FILE_NAME, "r"); // Modo lectura
	printf("los contenedores son:\n");
    while (fgets(buffer, 100, fichero)){//imprime linea por linea del txt mostrando los contenedores y su estado
        strtok(buffer, "\n");
        printf("%s\n", buffer);
    }
	fclose(fichero);//se cierra el fichero
	pthread_mutex_unlock(&mutex);//se desbloquea el mutex
	/*
	pid_t pid;
	pid = fork();  // Se crea un proceso para poder ejecutar el execvp
	if(pid == 0){  // Proceso hijo
		if(execvp(args[0], args) < 0){
			puts("Error.");
		}
	}
	else{
		wait(NULL);  // El padre espera al hijo
	}*/
}

void *stopContainer(void *params){  // Funcion del hilo que detiene los contenedores
	char *args[] = {"sudo", "docker", "stop", finalCommand[1], NULL};  // Detiene un contenedor dado su nombre
	char buffer[100];         // Aquí vamos a ir almacenando cada línea del txt
	char *listContenedores[100];  // tokens de cada linea
	char imagen[100];  // almacenara la imagen del contenedor
	bool existeContenedor = false; //variable que dira si el contendor existe o no en el txt
	int lineDelete = 0;//contador de la linea a eliminar en el txt
	int line = 0;//contador de la linea a eliminar en el txt
	pthread_mutex_lock(&mutex);//bloqueo del mutex
	FILE *fichero = fopen(FILE_NAME, "r"); // Modo lectura
    while (fgets(buffer, 100, fichero)){
        strtok(buffer, "\n");
		cadenaTokens(buffer, listContenedores); // cada linea que leer buffer se separa en tokens en list contenedores
		if(strcmp(listContenedores[1], finalCommand[1]) == 0){// compara si el nombre del contenedor a eliminar ya existe
			if(strcmp(listContenedores[0], "corriendo") == 0){// compara si el contenedor a eliminar ya esta detenido para eliminarlo
				existeContenedor = true;//el contenedor existe en el txt
				strcpy(imagen,listContenedores[2]);//almacena la imagen del contenedor a detener
				break;
			}
		}
    }
	fclose(fichero);//se cierra el fichero
	pthread_mutex_unlock(&mutex);//desbloqueo del mutex
	if (existeContenedor==true){//si el contenedor existe
		pthread_mutex_lock(&mutex);//se hace bloqueo con el mutex
		fichero = fopen(FILE_NAME, "a"); 
		fprintf(fichero,"detenido %s %s\n",finalCommand[1],imagen);//se agrega el contenedor en estado detenido
		fclose(fichero);  // Se cierra el txt o fichero
		FILE *fichero = fopen(FILE_NAME, "r"); // Modo lectura
		while (fgets(buffer, 100, fichero)){
			strtok(buffer, "\n");
			cadenaTokens(buffer, listContenedores); // cada linea que leer buffer se separa en tokens en list contenedores
			if(strcmp(listContenedores[1], finalCommand[1]) == 0){
				if (strcmp(listContenedores[0], "corriendo") == 0){//busca el contenedor con el estado corriendo para elminarlo del txt
					break;
				}
			}
			lineDelete++;
		}
		fclose(fichero);
		fichero = fopen(FILE_NAME, "r"); // Modo lectura
		FILE *fichero2 = fopen("temp.txt", "w"); // Modo escritura
		while(fgets(buffer, 100, fichero)){
			if(line != lineDelete){
				fputs(buffer, fichero2);
			}
			line++;
		}
		fclose(fichero2);//se cierra el fichero
		fclose(fichero);//se cierra el fichero
		remove(FILE_NAME);//se remueve el txt anterior
		rename("temp.txt",FILE_NAME);//renombra el txt temporal por el del archivo 
		pthread_mutex_unlock(&mutex);//desbloqueo del mutex
		pid_t pid;
		pid = fork();  // Se crea el proceso
		if(pid == 0){ // Proceso hijo
			if(execvp(args[0], args) < 0){
				puts("Error.");
			}
		}
		else{ 
			wait(NULL);  // El padre espera al hijo
		}
	}
	else
	{
		puts("no existe o se encuentra ya detenido el contenedor");//el contenedor o no existe o ya estaba detenido
	}
}

void *deleteContainer(void *params){  // Funcion del hilo que elimina los contenedores
	char *args[] = {"sudo", "docker", "rm", finalCommand[1], NULL};  // Elimina un contenedor dado su nombre
	char buffer[100];         // Aquí vamos a ir almacenando cada línea del txt
	char *listContenedores[100];  // tokens de cada linea
	bool existeContenedor = false;//variable que dira si el contendor existe o no en el txt
	int lineDelete = 0;//contador de la linea a eliminar en el txt
	int line = 0;//contador de la linea a eliminar en el txt
	pthread_mutex_lock(&mutex);//bloqueo del mutex
	FILE *fichero = fopen(FILE_NAME, "r"); // Modo lectura
    while (fgets(buffer, 100, fichero)){
        strtok(buffer, "\n");//leemos linea por linea del txt en buffer
		cadenaTokens(buffer, listContenedores); // cada linea que leer buffer se separa en tokens en listcontenedores
		if(strcmp(listContenedores[1], finalCommand[1]) == 0){// compara si el nombre del contenedor a eliminar ya existe
			if(strcmp(listContenedores[0], "detenido") == 0){// compara si el contenedor a eliminar ya esta detenido para eliminarlo
				existeContenedor = true;//el contenedor existe
				break;
			}
		}
    }
	fclose(fichero);//cierra el fichero
	pthread_mutex_unlock(&mutex);//desbloqueo del mutex
	if (existeContenedor==true){//el contenedor existe
		pthread_mutex_lock(&mutex);//bloqueo del mutex
		FILE *fichero = fopen(FILE_NAME, "r"); // Modo lectura
		while (fgets(buffer, 100, fichero)){
			strtok(buffer, "\n");//buffer lee linea por linea del txt
			cadenaTokens(buffer, listContenedores); // cada linea que leer buffer se separa en tokens en list contenedores
			if(strcmp(listContenedores[1], finalCommand[1]) == 0){//compara si el nombre del contenedor es el mismo del txt al ingresado por el cliente
				if (strcmp(listContenedores[0], "detenido") == 0){//mira si el estado del contenedor es detenido para poderlo eliminar
					break;
				}
			}
			lineDelete++;
		}
		fclose(fichero);//cierra el fichero
		fichero = fopen(FILE_NAME, "r"); // Modo lectura
		FILE *fichero2 = fopen("temp.txt", "w"); // Modo escritura
		while(fgets(buffer, 100, fichero)){
			if(line != lineDelete){
				fputs(buffer, fichero2);
			}
			line++;
		}
		fclose(fichero2);//cierra el fichero
		fclose(fichero);//cierra el fichero
		remove(FILE_NAME);//elimina el txt anterior
		rename("temp.txt",FILE_NAME);//renombra el txt temporal por el del archivo
		pthread_mutex_unlock(&mutex);
		pid_t pid;
		pid = fork();  // Se crea el proceso
		if(pid == 0){  // Proceso hijo
			if(execvp(args[0], args) < 0){
				puts("Error.");
			}
		}
		else{
			wait(NULL);  // El padre espera al hijo
		}
	}
	else{
		puts("no existe en el txt o no se encuetra detenido");//no lo eliminara si no esta detenido o no existe
	}
}


void *executeCommandBasic(void *params){  // Funcion que ejecuta un comando digitado por el usuario
	pid_t pid;
	pid = fork();  // Se crea el proceso

	if(pid == 0){  // Proceso hijo
		if(execvp(finalCommand[0], finalCommand) < 0){  // Se ejecuta el comando que digito el usuario
			puts("Error.");
		}
	}
	else{
		wait(NULL);  // El padre espera al hijo
	}
}

int main(int argc , char *argv[]) {
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server, client;  // https://github.com/torvalds/linux/blob/master/tools/include/uapi/linux/in.h
	char client_message[2000];  // Mensaje del cliente
	char *listCommands[100];  // Funcion que contendra el comando separado por espacios
	pthread_t tid1, tid2, tid3, tid4, tid5;  // Creacion de 5 identificadores para 5 hilos
	pthread_attr_t attr;  // Atributos para los hilos

	
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	
	//Bind the socket to the address and port number specified
	if( bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if(client_sock < 0) {
		perror("accept failed");
	}
	puts("Connection accepted");

	pthread_attr_init(&attr);  // inicializacion de los atributos por defecto para los hilos

	while(1) {
		//accept connection from an incoming client
		memset(client_message, 0, 2000);
		int typeCommand = 0;  // Variable para saber que hilo llamar
		//Receive a message from client
		if(recv(client_sock, client_message, 2000, 0) < 0) {
			printf("Error al recibir la peticion.\n");
			//Send the message back to client
			send(client_sock, "Error.", 6, 0);
			return 1;
		}
		cadenaTokens(client_message, listCommands);
		FILE *fichero = fopen(FILE_NAME, "a"); //crea el txt vacio para poder tener inventario de los contenedores
		fclose(fichero);  // Se cierra el txt o fichero
		if(strcmp("create", listCommands[0]) == 0){  // Para crear un contenedor por default.
			typeCommand = 1;
			memcpy(finalCommand, listCommands, 100);  // Se pasa el comando a la variable global para ser usada por los hilos
		}
		else if(strcmp("list", listCommands[0]) == 0){  // Para listar contenedores.
			typeCommand = 2;
			memcpy(finalCommand, listCommands, 100);
		}
		else if(strcmp("stop", listCommands[0]) == 0){  // Para detener un contenedor por default.
			typeCommand = 3;
			memcpy(finalCommand, listCommands, 100);
		}
		else if(strcmp("delete", listCommands[0]) == 0){  // Para eliminar un contenedor por default.
			typeCommand = 4;
			memcpy(finalCommand, listCommands, 100);
		}
		else if(strcmp("sudo", listCommands[0]) == 0){  // Para ejecutar un comando digitado por el usuario.
			typeCommand = 5;
			memcpy(finalCommand, listCommands, 100);
		}

		switch (typeCommand)  // Un switch que escoge que hilo llamar dependiendo una entrada
		{
		case 1: // Hilo crear
			pthread_create(&tid1, &attr, createContainer, NULL);
			pthread_join(tid1, NULL);
			break;
		
		case 2: // Hilo listar
			pthread_create(&tid2, &attr, listContainer, NULL);
			pthread_join(tid2, NULL);
			break;
		
		case 3:  // Hilo detener
			pthread_create(&tid3, &attr, stopContainer, NULL);
			pthread_join(tid3, NULL);
			break;

		case 4: // Hilo eliminar
			pthread_create(&tid4, &attr, deleteContainer, NULL);
			pthread_join(tid4, NULL);
			break;

		case 5:  // Hilo ejecutar
			pthread_create(&tid5, &attr, executeCommandBasic, NULL);
			pthread_join(tid5, NULL);
			break;
		}
		
		send(client_sock, "Peticion recibida.", 18, 0); // Se le envia un mensaje al cliente cuando se recibe una peticion.
    }
	return 0;
}