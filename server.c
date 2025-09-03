#include <sys/file.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <ifaddrs.h>
#include <ctype.h>
#include <time.h>

pthread_mutex_t tsk_mutex;
pthread_cond_t tsk_thcond;

int taskquant = 0;


/*

0 - html
1 - js
2 - css
3 - ico

*/
long int get_text_bytes(char *str){
    return strlen(str);
}

long long int convert_int(char* nmr){
    long long int size_of_str = 0;
    int limit= strlen(nmr);
    for (int i = 0;i<limit;i++){
        if (48 <= nmr[i] && 57 >= nmr[i]){
            size_of_str = size_of_str*10 + (nmr[i] - 48);
        } 
    }
    return size_of_str;
}

char *mimeTypes[] = {"text/html","text/javascript","text/css","image/x-icon"};

//-------------estrutura das linked lists----------

typedef struct Net{
    char *addr;
    char *port;
} Net;
Net netinfo;

typedef struct Task{
    int tsk_socketfd_cliente;
    int tsk_socketfd_servidor;

    long int request_size_bytes;
    char *tsk_full_request; // buffer with all request contents
    char *tsk_headers_only;
    char *tsk_task_connection;
    char *tsk_what_frontend_wants; // to sem criatividade :)

    struct Task *tsk_next;
    struct Task *tsk_prev;
    
} Task;
Task *inicio = NULL;
Task *final  = NULL;

void addTask(long int req_size,char *full_request,char *header,char *typeConnection,char *what_frontend_wants,int socketfd_cliente,int socketfd_servidor){
    int testes =0;
    
    Task *newtask = malloc(sizeof(Task));
    newtask->tsk_task_connection = strdup(typeConnection);
    newtask->tsk_what_frontend_wants = strdup(what_frontend_wants);
    
    newtask->tsk_full_request = malloc(req_size+1);
    memcpy(newtask->tsk_full_request, full_request, req_size);    

    newtask->tsk_headers_only = strdup(header);
    newtask->tsk_socketfd_cliente = socketfd_cliente;
    newtask->tsk_socketfd_servidor = socketfd_servidor;
    newtask->request_size_bytes = req_size;
    newtask->tsk_next = NULL;
    if (final == NULL){
        inicio = newtask;
    }
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

    if ((status3 = listen(*socketfd, 100)) != 0){
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

long int get_file_bytes(char* path){
    FILE* arq = fopen(path, "r");
    long long int nmr_bytes = 0;
    while (getc(arq) != EOF) {
        nmr_bytes++;
    }
    rewind(arq);
    fclose(arq);
    return nmr_bytes;
}

void send_text(char *path,int socketfd_client, char* mimeType){
    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: " ;
    char header_buffer[4096];
    long int bytes = get_file_bytes(path);
    snprintf(header_buffer, 4096, "%s%s; charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %ld\r\n\r\n",header,mimeType,bytes);
    send(socketfd_client, header_buffer, strlen(header_buffer), 0);
    
    //--------- file sending part ----------

    int request_full_size = 0;
    FILE *arq = fopen(path, "r");
    char ch = 'a', file_buffer[8192];
    while (ch!=EOF) {
        if (request_full_size > 8190){
            ch = getc(arq);
            file_buffer[request_full_size] = ch;
            send(socketfd_client, file_buffer, strlen(file_buffer)-1, 0);
            request_full_size = 0;    
            memset(file_buffer, 0, 8192);
        }
        else {
            ch = getc(arq);
            file_buffer[request_full_size] = ch;
            request_full_size++;
        }
    }
    if (request_full_size>0){
        send(socketfd_client, file_buffer, strlen(file_buffer)-1, 0);
    }
    close(socketfd_client);
    rewind(arq);
    fclose(arq);
}

void send_file(char *path,int socketfd_client, char *mimeType){
    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: " ;
    char header_buffer[4096];
    long int bytes = get_file_bytes(path);
    snprintf(header_buffer, 4096, "%s%s; charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %ld\r\n\r\n",header,mimeType,bytes);
    send(socketfd_client, header_buffer, strlen(header_buffer), 0);
    //--------- file sending part ----------
    int request_full_size = 0;
    long int request_full_size_supremo = 0;
    FILE *arq = fopen(path, "rb");
    char ch = 'a', file_buffer[8192];

    while (request_full_size_supremo!=bytes) {
        if (request_full_size > 8190){
            file_buffer[request_full_size] = ch;
            send(socketfd_client, file_buffer, request_full_size, 0);
            request_full_size = 0;    
        }
        else {
            ch = getc(arq);
            file_buffer[request_full_size] = ch;
            request_full_size++;
            request_full_size_supremo++;
        }
    }
    if (request_full_size>0){
        send(socketfd_client, file_buffer, request_full_size, 0);
    }
    fclose(arq);
}

//--------------------------------------------------------


// ----------------- processamento do upload --------------

void recive_file(Task *temp){
    int testes =0;
    //printf("Teste %d\n",testes);testes++;
    char *temp1 = strdup(temp->tsk_headers_only);
    char *temp2 = strdup(temp->tsk_headers_only);
    char *temp4 = strdup(temp->tsk_full_request);

   // printf("Teste %d\n",testes);testes++;

    char *file_ptr = strstr(temp->tsk_full_request, "<<<FILE_DATA_START>>>");

    //printf("Teste %d\n",testes);testes++;
    char *buffer1 = strstr(temp2, "X-File-Size: ");
    char *buffer2 = strstr(temp2, "X-File-Name: ");
    
    //printf("Teste %d\n",testes);testes++;

    char tempr1[8192],tempr2[8192];
    strcpy(tempr1, buffer1);
    strcpy(tempr2, buffer2);

    //printf("Teste %d\n",testes);testes++;

    char *content_lenght = strtok(tempr1, "\n");
    char *uploaded_file_name = strtok(tempr2, "\n");

    //printf("Teste %d\n",testes);testes++;

    uploaded_file_name+=13;
    uploaded_file_name[strlen(uploaded_file_name)-1] = '\0';

    //printf("Teste %d\n",testes);testes++;

    int header_lenght =  get_text_bytes(temp->tsk_headers_only) ;
    //printf("Teste %d\n",testes);testes++;
    long int file_buffer_lenght = temp->request_size_bytes - header_lenght - 39;
    //printf("Teste %d\n",testes);testes++;
    int true_size_file = convert_int(content_lenght);
    //printf("Teste %d\n",testes);testes++;
    char file_location[4096];
    //printf("Teste %d\n",testes);testes++;
    strcpy(file_location, uploaded_file_name);
    //printf("Teste %d\n",testes);testes++;
    char *tempptr = strstr(file_location,".");

    

    tempptr[0] = '\0';
    mkdir(uploaded_file_name,0755);
    snprintf(file_location, 4096, "%s/%s", uploaded_file_name,uploaded_file_name);

    FILE *arq = fopen(file_location, "wb");
    int arquivofd = fileno(arq);
    flock(arquivofd, LOCK_EX);
    //printf("%s\n",temp->tsk_headers_only);
    file_ptr+=22;
    fwrite(file_ptr, 1, file_buffer_lenght, arq);
    if ((temp->request_size_bytes - header_lenght - 39)  < true_size_file) {
        printf("Starting Big Upload...\n");
        while ((temp->request_size_bytes - header_lenght)  < true_size_file) {
            char buffer_socket[8192];
            int recv_size = recv(temp->tsk_socketfd_cliente, buffer_socket, 8192, 0);
            if (recv_size == 0) break;
            temp->request_size_bytes += recv_size;
            fwrite(buffer_socket, 1, recv_size, arq);
        }
        printf("Upload finished! :D\n");
    }
    fclose(arq);
    flock(arquivofd, LOCK_UN);
    send(temp->tsk_socketfd_cliente, "HTTP/1.1 200 OK", 16, 0);
    //printf("-------------------------pinto gay inicio-------------------------\n%s\n-------------------------pinto gay final----------
    // ---------------\n%ld\n%ld\n",temp->tsk_headers_only,get_text_bytes(temp->tsk_headers_only),get_text_bytes(full_content_lenght));
    close(temp->tsk_socketfd_cliente);
    free(temp1);
    free(temp2);
    free(temp4);
}

// -------------------------------------------------------

void *routine(void* arg){
    while (1) {
        pthread_mutex_lock(&tsk_mutex);
        while (taskquant == 0){
            pthread_cond_wait(&tsk_thcond, &tsk_mutex);
        }
        Task *temp = inicio;
        printf("[Request type: %s][Requested: %s][SocketFD: %d]\n\n",temp->tsk_task_connection, temp->tsk_what_frontend_wants,temp->tsk_socketfd_cliente);
        taskquant--;

        if (inicio->tsk_next == NULL){
            inicio  = NULL;
            final = NULL;
        }

        else {
            inicio = inicio->tsk_next;
        }
        pthread_mutex_unlock(&tsk_mutex);
        // -------------  processamento de requisiçoes------------------
        if (strcmp(temp->tsk_task_connection, "GET") == 0){
            if (strcmp(temp->tsk_what_frontend_wants,"/") == 0){
                send_text("www/index.html", temp->tsk_socketfd_cliente,mimeTypes[0]);
                close(temp->tsk_socketfd_cliente);
            }
            
            else if (strcmp(temp->tsk_what_frontend_wants,"/script.js") == 0){
                send_text("www/script.js", temp->tsk_socketfd_cliente,mimeTypes[1]);
                close(temp->tsk_socketfd_cliente);
            }

            else if (strcmp(temp->tsk_what_frontend_wants,"/style.css") == 0){
                send_text("www/style.css",temp->tsk_socketfd_cliente,mimeTypes[2]);
                close(temp->tsk_socketfd_cliente);
            }

            else if (strcmp(temp->tsk_what_frontend_wants,"/favicon.ico") == 0){
                send_file("www/favicon.ico",temp->tsk_socketfd_cliente,mimeTypes[3]);
                close(temp->tsk_socketfd_cliente);
            }
        }

        else if (strcmp(temp->tsk_task_connection, "POST") == 0){
            if (strcmp(temp->tsk_what_frontend_wants,"/upload") == 0){
                recive_file(temp);
            }
        }

        else if (strcmp(temp->tsk_task_connection, "OPTIONS") == 0) {
            char buffer[4096];
            snprintf(buffer, 4096, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST, OPT\r\nIONSAccess-Control-Allow-Headers: Content-Type\r\nContent-Length: 0");
            send(temp->tsk_socketfd_cliente, buffer, strlen(buffer)-1, 0);
            close(temp->tsk_socketfd_cliente);
        }
        //--------------------------------------
        free(temp->tsk_task_connection);
        free(temp->tsk_what_frontend_wants);
        free(temp->tsk_full_request);
        free(temp);
        pthread_cond_broadcast(&tsk_thcond);
    }
}

//-----------------------------------------------

/*




1 - html

2 - upload

*/

void setIp(char *port){
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            else if (strcmp(ifa->ifa_name, "wlp3s0")==0 || strcmp(ifa->ifa_name, "wlan0") == 0 || strcmp(ifa->ifa_name, "enp0s25")==0) {
                netinfo.addr = strdup(host);
                netinfo.port = strdup(port);
            }
        }
    }
}

int main(){
    pthread_mutex_init(&tsk_mutex, NULL);
    pthread_cond_init(&tsk_thcond, NULL);
    pthread_t tid[8];

    int socketfd,socket_clientfd;

    setIp("9988");

    struct sockaddr_storage client_conf;
    memset(&client_conf, 0, sizeof(client_conf));
    socklen_t size = sizeof(client_conf);
    int status = create_socket(&socketfd);
    char buffer[8192],clone_buffer[8192], *headers,*tipo_conexao,*nome_arquivo;
    printf("http://%s:%s\n\n",netinfo.addr,netinfo.port);

    
    for (int i = 0; i<8; i++) {
        pthread_create(&tid[i], NULL, routine, NULL);
    }

    while (1) {
        socket_clientfd = accept(socketfd, (struct sockaddr *)&client_conf, &size);
        int req_size = recv(socket_clientfd,buffer, 8192, 0);
        strcpy(clone_buffer, buffer);
        
        headers = strdup("temporario...");

        tipo_conexao = strdup(http_parser_of_type_request(buffer));
        nome_arquivo = strdup(http_parser_of_what_do_frontend_wants(buffer));
        if (strcmp(nome_arquivo, "/upload") == 0){
            headers = strstr(clone_buffer, "<<<HEADER_END>>>");
            headers[0] = '\0';
            //printf("%s",buffer);

        }
        pthread_mutex_lock(&tsk_mutex);
        addTask(req_size,buffer,clone_buffer,tipo_conexao, nome_arquivo, socket_clientfd,socketfd);
        taskquant++;
        pthread_mutex_unlock(&tsk_mutex);
        pthread_cond_signal(&tsk_thcond);
    }
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