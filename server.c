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
#include <sqlite3.h>
#include <locale.h>

#define SIZE_BUFFER 2 * 1024 * 1024
char *mimeTypes[] = {"text/html","text/javascript","text/css","image/x-icon"};


char cache_html[SIZE_BUFFER],cache_js[SIZE_BUFFER],cache_css[SIZE_BUFFER],cache_icon[SIZE_BUFFER];
long int size_html,size_js,size_css,size_icon;

pthread_mutex_t tsk_mutex;
pthread_cond_t tsk_thcond;

int taskquant = 0;

void setup_caches(){
    char ch = 'a';
    long int contador = 0;
    FILE *arq = fopen("www/index.html", "rb");
    while ((ch = getc(arq))!=EOF) {
        cache_html[contador] = ch;
        contador++;
    }
    size_html = contador;
    fclose(arq);
    
    contador = 0;
    arq = fopen("www/script.js", "rb");
    while ((ch = getc(arq))!=EOF) {
        cache_js[contador] = ch;
        contador++;
    }
    size_js = contador;
    fclose(arq);

    contador = 0;
    arq = fopen("www/style.css", "rb");
    while ((ch = getc(arq))!=EOF) {
        cache_css[contador] = ch;
        contador++;
    }
    size_css = contador;
    fclose(arq);

    contador = 0;
    arq = fopen("www/favicon.ico", "rb");
    while ((ch = getc(arq))!=EOF) {
        cache_icon[contador] = ch;
        contador++;
    }
    size_icon = contador;
    fclose(arq);
}
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


char db_nome[] = "storage.db";
sqlite3 *db;
//--------------------- DATABASE -----------------
void write_on_db(char *filename,char *file_path,char *mimetype,long long int file_size){
    sqlite3_open(db_nome,&db);
    sqlite3_stmt *comand;
    sqlite3_prepare_v2(db, "INSERT INTO files (filename, file_path, mimeType, size_bytes) VALUES ( ?, ?, ?, ?)", -1, &comand, NULL);
    sqlite3_bind_text(comand, 1, filename, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(comand, 2, file_path, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(comand, 3, mimetype, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(comand, 4, file_size);
    sqlite3_step(comand);
    sqlite3_finalize(comand);
    sqlite3_close(db);
}
// ------------------------------------------------

//-------------estrutura das linked lists----------

typedef struct Net{
    char *addr;
    char *port;
} Net;
Net netinfo;

typedef struct Task{
    int tsk_socketfd_cliente;
    int tsk_socketfd_servidor;

    long long int request_size_bytes;
    char *tsk_full_request; // buffer with all request contents
    char *tsk_headers_only;
    char *tsk_task_connection;
    char *tsk_what_frontend_wants; // to sem criatividade :)

    struct Task *tsk_next;
    
} Task;
Task *inicio = NULL;
Task *final  = NULL;

void addTask(long long int req_size,char *full_request,char *header,char *typeConnection,char *what_frontend_wants,int socketfd_cliente,int socketfd_servidor){
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
        sleep(2);
        exit(1);
        return status1;
    }
    
    *socketfd = socket(sockinfo_list->ai_family, sockinfo_list->ai_socktype, sockinfo_list->ai_protocol);
    
    if ((status2 = bind(*socketfd, sockinfo_list->ai_addr, sockinfo_list->ai_addrlen)) != 0){
        printf("err bind\n");
        close(*socketfd);
        sleep(2);
        exit(2);
        return status2;
    }

    if ((status3 = listen(*socketfd, 10000)) != 0){
        printf("err listen\n");
        close(*socketfd);
        sleep(2);
        exit(3);
        return status3;
    }
    return 0;
}

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

void send_file(char *path,int socketfd_client, int mimeType_nmbr){
    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: " ;
    char *mimeTypes[] = {"text/html","text/javascript","text/css","image/x-icon"};
    char header_buffer[4096];
    long int bytes = get_file_bytes(path);
    snprintf(header_buffer, 4096, "%s%s; charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %ld\r\n\r\n",header, mimeTypes[mimeType_nmbr],bytes);
    if(send(socketfd_client, header_buffer, strlen(header_buffer), 0)<= 0 ){
        return;
    }
    //--------- file sending part ----------
    if (mimeType_nmbr == 0){
        send(socketfd_client,cache_html, size_html, 0);
    }
    if (mimeType_nmbr == 1){
        send(socketfd_client, cache_js, size_js, 0);
    }
    if (mimeType_nmbr == 2){
        send(socketfd_client, cache_css, size_css, 0);
    }
    if (mimeType_nmbr == 3){
        send(socketfd_client, cache_icon, size_icon, 0);
    }
}
// char *mimeTypes[] = {"text/html","text/javascript","text/css","image/x-icon"};

//--------------------------------------------------------


// ----------------- processamento do upload --------------

void recive_file(Task *temp){
    int testes =0;
    int header_lenght =  get_text_bytes(temp->tsk_headers_only) ;
    char *temp1 = strdup(temp->tsk_headers_only);
    char *temp2 = strdup(temp->tsk_headers_only);


   // printf("Teste %d\n",testes);testes++;

    char *file_ptr = strstr(temp->tsk_full_request, "<<<FILE_DATA_START>>>");

    char *buffer1 = strstr(temp2, "X-File-Size: ");
    char *buffer2 = strstr(temp2, "X-File-Name: ");
    char *buffer3 = strstr(temp2, "FILETYPE: ");
    char tempr1[header_lenght],tempr2[header_lenght], tempr3[header_lenght];
    strcpy(tempr1, buffer1);
    strcpy(tempr2, buffer2);
    strcpy(tempr3, buffer3);

    char *file_size_str = strtok(tempr1, "\n");
    char *uploaded_file_name = strtok(tempr2, "\n");
    char *mimeType_db = (strtok(tempr3, "\n"))+10;


    uploaded_file_name+=13;
    uploaded_file_name[strlen(uploaded_file_name)-1] = '\0';


    long long int file_buffer_lenght = temp->request_size_bytes - header_lenght - 39;
    long long int total_size = file_buffer_lenght;
    long long int true_request_file_size = convert_int(file_size_str);
    char file_location1[4096],file_location2[8192],file_name_noExt[4096];
    strcpy(file_name_noExt, uploaded_file_name);
    char *tempptr = strstr(file_name_noExt,".");

    tempptr[0] = '\0';

    snprintf(file_location1, 4096, "uploads/%s", file_name_noExt);
    snprintf(file_location2, 8192, "%s/%s", file_location1, uploaded_file_name);

    mkdir(file_location1,0777);
    
    FILE *arq = fopen(file_location2, "wb");
    int arquivofd = fileno(arq);
    flock(arquivofd, LOCK_EX);
    file_ptr+=22;
    printf("Upload Recived...\n");
    fwrite(file_ptr, 1, file_buffer_lenght, arq);
    if (total_size < true_request_file_size) {
        printf("Starting Big Upload...\nFile Name: %s\n\n",file_name_noExt);
        while (total_size  < true_request_file_size) {
            char buffer_socket[2 * 1024 * 1024];
            long long int recv_size = recv(temp->tsk_socketfd_cliente, buffer_socket,  2 * 1024 * 1024 , 0);
            if (recv_size <= 0) break;
            total_size+=recv_size;
            fwrite(buffer_socket, 1, recv_size, arq);
        }
        float file_size_converted;
        if (1024 * 1024 * 1024 < total_size){
            fclose(arq);
            flock(arquivofd, LOCK_UN);
            file_size_converted = total_size/(1024.0 * 1024.0 * 1024.0);
            send(temp->tsk_socketfd_cliente, "HTTP/1.1 200 OK", 15, 0);
            close(temp->tsk_socketfd_cliente);
            printf("Upload finished! :D\nFile Name: \"%s\"\nFile Location: \"%s\"\nFile size: %.2f GBs\n\n",file_name_noExt,file_location2,file_size_converted);
        }
        else {
            fclose(arq);
            flock(arquivofd, LOCK_UN);
            file_size_converted = total_size/(1024.0 * 1024.0);
            send(temp->tsk_socketfd_cliente, "HTTP/1.1 200 OK", 15, 0);
            close(temp->tsk_socketfd_cliente);
            printf("Upload finished! :D\nFile Name: \"%s\"\nFile Location: \"%s\"\nFile size: %.2f MBs\n\n",file_name_noExt,file_location2,file_size_converted); 
        }
    }
    else{
        float file_size_converted = total_size/(1024.0);
        fclose(arq);
        flock(arquivofd, LOCK_UN);
        printf("Upload finished! :D\nFile Name: \"%s\"\nFile Location: \"%s\"\nFile size: %.2f KBs\n\n",file_name_noExt,file_location2,file_size_converted); 
        send(temp->tsk_socketfd_cliente, "HTTP/1.1 200 OK", 15, 0);
        close(temp->tsk_socketfd_cliente);
    }
    write_on_db(file_name_noExt,file_location2,mimeType_db,total_size);    
    free(temp1);
    free(temp2);
}

// -------------------------------------------------------
void *routine(void* arg){
    int index = *(int *)arg;
    while (1) {
        pthread_mutex_lock(&tsk_mutex);
        while (taskquant == 0){
            pthread_cond_wait(&tsk_thcond, &tsk_mutex);
        }
        Task *temp = inicio;
        printf("[Request type: %s][Requested: %s][SocketFD: %d][Thread: %d]\n\n",temp->tsk_task_connection, temp->tsk_what_frontend_wants,temp->tsk_socketfd_cliente,index);
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
                send_file("www/index.html", temp->tsk_socketfd_cliente,0);
                close(temp->tsk_socketfd_cliente);
            }
            
            else if (strcmp(temp->tsk_what_frontend_wants,"/script.js") == 0){
                send_file("www/script.js", temp->tsk_socketfd_cliente,1);
                close(temp->tsk_socketfd_cliente);
            }

            else if (strcmp(temp->tsk_what_frontend_wants,"/style.css") == 0){
                send_file("www/style.css",temp->tsk_socketfd_cliente,2);
                close(temp->tsk_socketfd_cliente);
            }

            else if (strcmp(temp->tsk_what_frontend_wants,"/favicon.ico") == 0){
                send_file("www/favicon.ico",temp->tsk_socketfd_cliente,3);
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
        free(temp->tsk_headers_only);
        free(temp->tsk_task_connection);
        free(temp->tsk_what_frontend_wants);
        free(temp->tsk_full_request);
        free(temp);
        pthread_cond_broadcast(&tsk_thcond);
    }
}

//----------------------------------------------

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
int arquivo_existe(char *nome) {
    struct stat buffer;
    return (stat(nome, &buffer) == 0);
}

void create_db(){
    char *sql = "CREATE TABLE IF NOT EXISTS files ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "filename TEXT NOT NULL,"
            "file_path TEXT NOT NULL,"
            "mimeType TEXT NOT NULL,"
            "size_bytes INTEGER,"
            "upload_date DATETIME DEFAULT CURRENT_TIMESTAMP);";
          
    if (arquivo_existe(db_nome) == 0){
        sqlite3_open(db_nome,&db);
        sqlite3_exec(db, sql, 0, 0, 0);
    }
    else {
        char comands[] = "DROP TABLE files";
        sqlite3_open(db_nome,&db);
        sqlite3_exec(db, comands, 0,0,0);
        sqlite3_exec(db, sql, 0, 0, 0);
    }
}

int main(){
    setlocale(LC_ALL, "");
    setup_caches();
    pthread_mutex_init(&tsk_mutex, NULL);
    pthread_cond_init(&tsk_thcond, NULL);
    pthread_t tid[8];

    int socketfd,socket_clientfd;

    setIp("9988");

    struct sockaddr_storage client_conf;
    memset(&client_conf, 0, sizeof(client_conf));
    socklen_t size = sizeof(client_conf);
    int status = create_socket(&socketfd);
    
    printf("http://%s:%s\n\n",netinfo.addr,netinfo.port);
    int cond_create_db;

    int *index_threads = malloc(8 * sizeof(int));
    for (int i = 0; i<8; i++) {
        index_threads[i] = i;
        pthread_create(&tid[i], NULL, routine, &index_threads[i]);
    }

    if ((cond_create_db = mkdir("uploads", 0755)) == 0){
        create_db();
        sqlite3_close(db);
    }
    else{
        sqlite3_open(db_nome, &db);
        sqlite3_close(db);
    }
    int teste = 0;
    
    while (1) {
        
        char buffer[SIZE_BUFFER],temp_buffer_cpy[SIZE_BUFFER],clone_buffer[SIZE_BUFFER], *headers;
        socket_clientfd = accept(socketfd, (struct sockaddr *)&client_conf, &size);
        long long int req_size;
        if ((req_size = recv(socket_clientfd,buffer, SIZE_BUFFER, 0)) <=0){
            close(socket_clientfd);
            continue;
        }
        
        strcpy(clone_buffer, buffer);
        strcpy(temp_buffer_cpy, buffer);

        char *tkn1 = strtok(temp_buffer_cpy, " ");
        
        char *tipo_conexao = strdup(tkn1);
        tkn1 = strtok(NULL, " ");
        char *nome_arquivo = strdup(tkn1);

        if (strcmp(nome_arquivo, "/upload") == 0){
            headers = strstr(clone_buffer, "<<<HEADER_END>>>");
            headers[0] = '\0';
        }
        pthread_mutex_lock(&tsk_mutex);
        addTask(req_size,buffer,clone_buffer,tipo_conexao, nome_arquivo, socket_clientfd,socketfd);
        taskquant++;
        pthread_mutex_unlock(&tsk_mutex);
        memset(buffer, 0, SIZE_BUFFER);
        memset(temp_buffer_cpy, 0, SIZE_BUFFER);
        memset(clone_buffer, 0, SIZE_BUFFER);
        free(tipo_conexao);
        free(nome_arquivo);
        pthread_cond_signal(&tsk_thcond);
       
    }
}

/*
GET / HTTP/3
Host: web.whatsapp.com
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:142.0) Gecko/20100101 Firefox/142.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate, br, zstd
Alt-Used: web.whatsapp.com
Connection: keep-alive
Cookie: wa_ul=0890278e-ca51-4019-a196-6657fb488aea; wa_web_lang_pref=pt_BR
Upgrade-Insecure-Requests: 1
Sec-Fetch-Dest: document
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: none
Sec-Fetch-User: ?1
Priority: u=0, i
TE: trailers
*/