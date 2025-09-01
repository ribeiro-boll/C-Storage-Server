
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

pthread_mutex_t tsk_mutex;
pthread_cond_t tsk_thcond;

int taskquant = 0;

//-------------estrutura das linked lists----------

typedef struct Net{
    char *addr;
    char *port;
} Net;
Net netinfo;

typedef struct Task{
    int tsk_socketfd_cliente;
    int tsk_socketfd_servidor;

    char *tsk_task_connection;
    char *tsk_what_frontend_wants; // to sem criatividade :)

    struct Task *tsk_next;
    struct Task *tsk_prev;
    
} Task;
Task *inicio = NULL;
Task *final  = NULL;

void addTask(char *typeConnection,char *what_frontend_wants,int socketfd_cliente,int socketfd_servidor){
    Task *newtask = malloc(sizeof(Task));
    newtask->tsk_task_connection = strdup(typeConnection);
    newtask->tsk_what_frontend_wants = strdup(what_frontend_wants);
    newtask->tsk_socketfd_cliente = socketfd_cliente;
    newtask->tsk_socketfd_servidor = socketfd_servidor;
    if (final == NULL)
        inicio = newtask;
    else
        final->tsk_next = newtask;
    final = newtask;
}

// -----------------------------------------------------

// ---------------processamento das conexões------------

int create_socket(int *socketfd){
    struct addrinfo sockinfo;
    memset(&sockinfo, 0, sizeof(sockinfo));
    struct addrinfo *sockinfo_list;

    int status1,status2,status3;
    
    sockinfo.ai_family = AF_INET;
    sockinfo.ai_flags = AI_PASSIVE;
    sockinfo.ai_socktype = SOCK_STREAM;

    if((status1 = getaddrinfo(netinfo.addr,netinfo.port,&sockinfo,&sockinfo_list)) != 0){
        printf("err getaddr\n");
        return status1;
    }
    
    *socketfd = socket(sockinfo_list->ai_family, sockinfo_list->ai_socktype, sockinfo_list->ai_protocol);
    
    if ((status2 = bind(*socketfd, sockinfo_list->ai_addr, sockinfo_list->ai_addrlen)) != 0){
        printf("err bind\n");
        close(*socketfd);
        return status2;
    }

    if ((status3 = listen(*socketfd, 10)) != 0){
        printf("err listen\n");
        close(*socketfd);
        return status3;
    }
    return 0;
}



char* http_parser_of_type_request(char buffer[]){ // mais facil ne kkkkkkkk
    char ch1[1] = {'\n'},ch2[1] = {' '},*temp = strdup(buffer);;
    char *token = strtok(temp, ch1);
    
    return token;
}


char* http_parser_of_what_do_frontend_wants(char buffer[]){
    char ch1[1] = {'\n'},ch2[1] = {' '},*temp = strdup(buffer);
    char *token = strtok(temp, ch1);
    token = strtok(NULL, ch2);

    return token;
}

//--------------------------------------------------------


// ----------------- processamento do html----------------


// -------------------------------------------------------

void *routine(void* arg){
    pthread_mutex_lock(&tsk_mutex);
    while (taskquant == 0){
        pthread_cond_wait(&tsk_thcond, &tsk_mutex);
    }
    if (strcmp(inicio->tsk_task_connection, "GET") == 0){}
    
    else if (strcmp(inicio->tsk_task_connection, "POST") == 0){}
    
    
}

/*

1 - html

2 - upload

*/


int main(){
    pthread_mutex_init(&tsk_mutex, NULL);
    pthread_cond_init(&tsk_thcond, NULL);
    pthread_t tid[8];

    int socketfd,socket_clientfd;

    netinfo.addr = "127.0.0.1";
    netinfo.port = "8000";

    struct sockaddr_storage client_conf;
    memset(&client_conf, 0, sizeof(client_conf));
    socklen_t size = sizeof(client_conf);
    int status = create_socket(&socketfd);
    char buffer[4096], *clone,*tipo_conexao,*nome_arquivo;
    printf("http://%s:%s\n\n",netinfo.addr,netinfo.port);

    
    for (int i = 0; i<8; i++) {
        //pthread_create(tid[i], NULL, routine, inicio);
    }

    while (1) {
        socket_clientfd = accept(socketfd, (struct sockaddr *)&client_conf, &size);
        recv(socket_clientfd,buffer, 4096, 0);
        clone = strdup(buffer);
        
        tipo_conexao = strdup(http_parser_of_type_request(buffer));
        nome_arquivo = strdup(http_parser_of_what_do_frontend_wants(buffer));
        
        addTask(tipo_conexao, nome_arquivo, socket_clientfd,socketfd);
        taskquant++;
    }
    Task *temp = inicio;
    getc(stdin);
}



// GET / HTTP/1.1
// Host: 127.0.0.1:8000
// User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:142.0) Gecko/20100101 Firefox/142.0
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// Accept-Language: en-US,en;q=0.5
// Accept-Encoding: gzip, deflate, br, zstd
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// Sec-Fetch-Dest: document
// Sec-Fetch-Mode: navigate
// Sec-Fetch-Site: none
// Sec-Fetch-User: ?1
// Priority: u=0, i