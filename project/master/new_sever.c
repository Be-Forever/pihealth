#include"master.h"
#include"common/common.h"


LinkedList client_link[20];
int INS;
int master_port;
int msg_port;
int client_port;
int udp_port;
int data_port;
char *config = "./pihealthd.conf";
char *pilog = "./pi.log";

void *MONITOR(void *client_link){
    LinkedList now_link = *(LinkedList *)client_link;
    sleep(10);
    while(1){
        Node *p = (Node *)malloc(sizeof(Node));
        p = now_link;
        int cnt = counter(now_link);
        int num = 0;
        pthread_rwlock_rdlock(&rwlock);
        while(p->next != NULL && num <= cnt){
            char nowip[20];
            strcpy(nowip, inet_ntoa(p->next->cl_addr.sin_addr));
            pthread_rwlock_unlock(&rwlock);
            //printf("Sendto %s signal\n", nowip);
            int msgfd;
            if((msgfd = socket_connect(nowip, msg_port)) > 0){
                char dirname[50] = {0};
                strcpy(dirname, nowip);
                mkdir(dirname, 0777);
                char signal[10] = {0};
                for(int i = 100; i < 106; i++){
                    int tmp = i;
                    memset(signal, 0, sizeof(signal));
                    sprintf(signal, "%d", tmp);
                    //printf("signal: %s\n", signal);
                    if(send(msgfd, signal, strlen(signal), 0) < 0){
                        //printf("send signal FAILED\n");
                    }
                    char logname[10] = {0};
                    switch(i){
                        case 100:
                            strcpy(logname, "cpu");
                            break;
                        case 101:
                            strcpy(logname, "mem");
                            break;
                        case 102:
                            strcpy(logname, "disk");
                            break;
                        case 103:
                            strcpy(logname, "pro");
                            break;
                        case 104:
                            strcpy(logname, "sys");
                            break;
                        case 105:
                            strcpy(logname, "user");
                            break;
                    }
                    char logad[50] = {0};
                    sprintf(logad, "%s/", dirname);
                    strcat(logad, logname);
                    strcat(logad, ".log");
                    FILE *file;
                    file = fopen(logad, "a+");
                   
                    char buf[10240] = {0};
                    //printf("recving file\n");
                    while(recv(msgfd, buf, sizeof(buf), 0) > 0){
                        if(strcmp(buf, "end") == 0)break;
                        write_Pi_log(logad, "%s", buf);
                        write_Pi_log(pilog, "Recv %d Files From %s", strlen(buf), nowip);
                        memset(buf, 0, sizeof(buf));
                    }
                    fclose(file);
                    sleep(1.5);
                }
            }
            close(msgfd);
            delete(now_link, nowip);
        }
        pthread_rwlock_unlock(&rwlock);
    }
}


void *HEART(){
    while(1){
        fd_set rd;
        FD_ZERO(&rd);
        int maxfd = 0;
        int len = sizeof(int);
        struct timeval tm;
        tm.tv_sec = 0;
        tm.tv_usec = 300000;
        for(int i = 0; i < INS; i++){
            Node *p = client_link[i];
            while(p->next != NULL){
                int clientfd;
                if(clientfd = socket(AF_INET, SOCK_STREAM, 0) < 0){
                    write_Pi_log(pilog, "Socket Create Failed");
                }
                unsigned long ul = 1;
                ioctl(clientfd, FIONBIO, &ul);
                
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = p->next->cl_addr.sin_addr.s_addr;
                addr.sin_port = htons(client_port);
                if(connect(clientfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
                    //write_Pi_log(pilog, "Connect: %s", strerror(errno));
                }
                FD_SET(clientfd, &rd);
                if(maxfd < clientfd){
                    maxfd = clientfd;
                }
                p->next->fd = clientfd;
                p = p->next;
            }
            if(select(maxfd + 1, &rd, &rd, NULL, &tm) < 0){
                write_Pi_log(pilog, "Select: %s", strerror(errno));
            }else{
                for(int i = 0; i < INS; i++){
                    Node *q = client_link[i];
                    while(q->next != NULL){
                        if(FD_ISSET(q->next->fd, &rd) == 0){
                            close(q->next->fd);
                            client_link[i] = delete(client_link[i], inet_ntoa(q->next->cl_addr.sin_addr));
                        }else{
                            int erro = -1;
                            getsockopt(q->next->fd, SOL_SOCKET, SO_ERROR, &erro, (socklen_t *)&len);
                            if(erro == 0){
                                close(q->next->fd);
                                client_link[i] = delete(client_link[i], inet_ntoa(q->next->cl_addr.sin_addr));
                            }else{
                                close(q->next->fd);
                                q = q->next;
                            }
                        }
                    }
                }
            }
        }
    }
}

void *database(){
    int datafd = socket_create(udp_port);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int len = sizeof(addr);
    while(1){
        int accfd = accept(datafd, (struct sockaddr *)&addr, &len);
        char data[256] = {0};
        if(recv(accfd, data, sizeof(data), 0) > 0){
            char ip[20] = {0};
            char nowtime[30] = {0};
            int types;
            char ty[5] = {0};
            char details[128] = {0};
            char level[10] = {0};
            time_t logtime = time(NULL);
            struct tm *tmp = localtime(&logtime);
            sprintf(nowtime, "%d.%02d.%02d %02d:%02d:%02d ", (1900 + tmp->tm_year), (1 + tmp->tm_mon), \
    tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
            strcpy(ip, inet_ntoa(addr.sin_addr));
            strncat(ty, data, 3);
            types = atoi(ty);
            int len = strlen(data);
            char temp[10] = {0};
            strncat(temp, &data[len - 5], 4);
            if(strcmp(temp, "note") == 0){
                strcpy(level, temp);
            }else{
                strcpy(level, "warning");
            }
            strncat(details, &data[3], len - 3);
            add_to_database(nowtime, ip, types, details, level);
        }
    }
}

void server(){
    int listenfd = socket_create(master_port);  //建立鏈接
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    
    char start_ip[20], end_ip[20], thread[5];
    get_conf_value(config, "thread", thread);
    get_conf_value(config, "start_ip", start_ip);
    get_conf_value(config, "end_ip", end_ip);
    INS = atoi(thread);
    //printf("end : %s start : %s\n", start_ip, end_ip);
    
    pthread_t pthread[INS];
    for(int i = 0; i < INS; i++){
        client_link[i] = init_link(client_link[i]);
        pthread_create(&pthread[i], NULL, MONITOR, (void*)&client_link[i]);
    }

    pthread_t pheart;
    pthread_create(&pheart, NULL, HEART, NULL);
    
    struct in_addr addrfrom;
    struct in_addr addrto;
    inet_aton(start_ip, &addrfrom);
    inet_aton(end_ip, &addrto);
    unsigned int nfrom = ntohl(addrfrom.s_addr);
    unsigned int nto = ntohl(addrto.s_addr);
    for(unsigned i = nfrom; i <= nto; i++){
        Node *node = (Node *)malloc(sizeof(Node));
        node->next = NULL;
        node->cl_addr.sin_addr.s_addr = htonl(i);
        insert(client_link[get_min(client_link, INS)], node);
    }
    pthread_t pdata;
    pthread_create(&pdata, NULL, database, NULL);
    while(1){
        int acceptfd;
        if((acceptfd = accept(listenfd, (struct sockaddr *)&client_addr, &len)) == -1){
            write_Pi_log(pilog, "Accept: %s", strerror(errno));
            //perror("accept");
            continue;
        }

        Node *p = (Node *)malloc(sizeof(Node));
        p->cl_addr = client_addr;
        p->fd = acceptfd;
        p->next = NULL;
        
        int mark = 0;
        for(int i = 0; i < INS; i++){
            if(check(client_link[i], client_addr.sin_addr.s_addr) == 1){
                mark = 1;
            }
        }

        if(mark != 1){
            //printf("%s insert to client_link[%d]\n", inet_ntoa(client_addr.sin_addr), get_min(client_link, INS));
            insert(client_link[get_min(client_link, INS)], p);
        }
        //printf("%s login\n", inet_ntoa(client_addr.sin_addr));
    }

}




int main(){
    char mport[5] = {0};
    char msport[5] = {0};
    char cport[5] = {0};
    char uport[5] = {0};
    char dport[5] = {0};
    get_conf_value(config, "master_port", mport);
    get_conf_value(config, "msg_port", msport);
    get_conf_value(config, "client_port", cport);
    get_conf_value(config, "UDP_port", uport);
    master_port = atoi(mport);
    msg_port = atoi(msport);
    client_port = atoi(cport);
    udp_port = atoi(uport);
    data_port = atoi(dport);
    write_Pi_log(pilog, "Server Start");
    server();
    return 0;
}
