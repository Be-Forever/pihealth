/*************************************************************************
	> File Name: sec_client.c
	> Author: 
	> Mail: 
	> Created Time: Sat Mar 16 14:09:38 2019
 ************************************************************************/

#include"common/common.h"

struct sm_msg{
    int heart_signal;  //whether the fork Hearted Success
    int flag;    //test times;
    int cnt;
    int heart;
    pthread_mutex_t sm_mutex;
    pthread_cond_t sm_ready;
};


char *pilog = "./pi.log";
char *config = "./Pihealth.conf";
char *share_memory = NULL;
char log_backup[50] = {0};
char logdir[50] = {0};
double dyaver;
struct sm_msg *msg;
pthread_mutexattr_t m_attr;
pthread_condattr_t c_attr;
char master_ip[20] = {0};

int get_file_size(char *filename){
    FILE *fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    return size;
}

void unzip(char *filename, char *outfile){
    FILE *file;
    uLong flen;
    unsigned char *fbuff = NULL;
    uLong ulen; 
    unsigned char *ubuff = NULL;

    if((file = fopen(filename, "rb")) == NULL){
        printf("Cannot Open %s\n", filename);
        return;
    }

    fread(&ulen, sizeof(uLong), 1, file);
    fread(&flen, sizeof(uLong), 1, file);
    if((fbuff = (unsigned char*)malloc(sizeof(unsigned char) * flen)) == NULL){
        printf("No Enough Memory For Old File\n");
        fclose(file);
        return;
    }
   
    fread(fbuff, sizeof(unsigned char), flen, file);
    if((ubuff = (unsigned char*)malloc(sizeof(unsigned char) * ulen)) == NULL){
        printf("No Enough Memory For New File\n");
        return;
    }
    if(uncompress(ubuff, &ulen, fbuff, flen) != Z_OK){
        printf("Unzip %s Failed\n", filename);
        return;
    }
    fclose(file);


    char unzip_name[100] = {0};
    int i;
    for(i = strlen(filename) - 1; i > 0; i--){
        if(filename[i] == '.')break;
    }
    strncat(unzip_name, filename, i);
    printf("unzip_name: %s\n", unzip_name);
    if((file = fopen(unzip_name, "wb")) == NULL){
        printf("Cannot Create %s\n", unzip_name);
        return;
    } 
    fwrite(ubuff, sizeof(unsigned char), ulen, file);
    fclose(file);
    free(ubuff);
    free(fbuff);
    remove(filename);
    write_Pi_log(pilog, "Send %d Files To Master", get_file_size(unzip_name));
    strcpy(outfile, unzip_name);
}


void backup(char *filename){
    int pos;
    FILE *in, *out;
    uLong inlen, outlen;
    char *inbuff = NULL;
    char *outbuff = NULL;
    int len = strlen(filename);
    for(int i = len - 1; i > 0; i--){
        if(filename[i] != '/'){
            continue;
        }else{
            pos = i + 1;
            break;
        }
    }
    char name[100] = {0};
    strncpy(name, &filename[pos], 3);
    time_t t;
    time(&t);
    struct tm *tmp_time = localtime(&t);
    char now_time[50] = {0};
    strftime(now_time, sizeof(now_time), "(%04Y-%02m-%02d %H:%M:%S)", tmp_time);
    strcat(name, now_time);
    printf("Compressname:%s\n", name);
    strcat(name, ".gz");
    char outfile[50] = {0};
    strncat(outfile, filename, pos);
    strcat(outfile, "/backup/");
    strcat(outfile, name);
    
    in = fopen(filename, "r");
    fseek(in, 0, SEEK_END);
    inlen = ftell(in);
    fseek(in, 0, SEEK_SET);

    inbuff = (char *)malloc(inlen * sizeof(char));
    fread(inbuff, sizeof(char), inlen, in);

    outlen = compressBound(inlen);
    outbuff = (char *)malloc(outlen * sizeof(char));
    if(compress(outbuff, &outlen, inbuff, inlen) != Z_OK){
        printf("Compress %s Failed\n", filename);
        write_Pi_log(pilog, "Compress %s Failed", filename);
    }
    fclose(in);

    out = fopen(outfile, "w+");

    fwrite(&inlen, sizeof(int), 1, out);
    fwrite(&outlen, sizeof(int), 1, out);
    fwrite(outbuff, sizeof(char), outlen, out);

    fclose(out);
    free(inbuff);
    free(outbuff);
}


void send_data(char *filename, int fd){
    FILE *file;
    if((file = fopen(filename, "r+")) == NULL){
        printf("Cannot Open %s while Sending\n", filename);
        return;
    }
    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    char *msg = (char *)malloc(sizeof(char) * (len + 5));
    fseek(file, 0, SEEK_SET);
    fread(msg, sizeof(char), len, file);
    fclose(file);
    //send msg to Server
    
    if(send(fd, msg, strlen(msg), 0) < 0){
        printf("Error in Sending...............\n");
    }else{
        remove(filename);
    }
    sleep(2);
}



void data_trans(char *signal, int fd){
    int type = atoi(signal);
    char logname[50] = {0};
    switch(type){
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
    // read the logname under logdir and log_backup
    DIR *dir;
    struct dirent *p;
    if((dir = opendir(logdir)) == NULL){
        printf("Error Open %s\n", logdir);
        return;
    }

    
    while((p = readdir(dir)) != NULL){
        if(p->d_name[0] == logname[0]){
            char sendfile[100] = {0};
            strcpy(sendfile, logdir);
            strcat(sendfile, "/");
            strcat(sendfile, p->d_name);
            printf("\nsendfile : %s\n", sendfile);
            send_data(sendfile, fd);
        }
    }

    closedir(dir);
    
    if((dir = opendir(log_backup)) == NULL){
        printf("Error Open %s\n", log_backup);
        return;
    }

    while((p = readdir(dir)) != NULL){
        if(p->d_name[0] == logname[0]){
            char sendfile[100] = {0};
            strcpy(sendfile, log_backup);
            strcat(sendfile, "/");
            strcat(sendfile, p->d_name);
            char *unzipfile;
            unzip(sendfile, unzipfile);
            printf("sendfile : %s\n", unzipfile);
            send_data(unzipfile, fd);
        }
    }
    send(fd, "end", 3, 0);
    closedir(dir);

}

void send_database(char *info){
    char uport[5] = {0};
    int udp_port;
    get_conf_value(config, "UDP_port", uport);
    udp_port = atoi(uport);

    int infofd = socket_connect(master_ip, udp_port);
    send(infofd, info, strlen(info), 0);
    close(infofd);
}


void sys_detect(int type){
    int time_i = 0;
    char src[50] = {0};
    sprintf(src, "src%d", type);
    char run[50] = {0};
    char buffer[4096] = {0};
    char logname[50] = {0};
    int maxsize;
    char max[20] = {0};
    get_conf_value(config, "maxsize", max);
    maxsize = atoi(max);
    get_conf_value(config, src, src);
    FILE *fstream = NULL;
    int times = 0;
    char temp[4] = {0};
    get_conf_value(config, "WriteEveryTime", temp);
    times = atoi(temp);
    int writetime;
    char tmp[4];
    get_conf_value(config, "writetime", tmp);
    writetime = atoi(tmp);
    switch(type){
        case 100: //cpu
            time_i = 5;
            sprintf(logname, "%s/cpu.log", logdir);
            break;
        case 101://mem
            time_i = 5;
            sprintf(logname, "%s/mem.log", logdir);
            break;
        case 102://disk
            time_i = 60;
            sprintf(logname, "%s/disk.log", logdir);
            break;
        case 103://pro
            time_i = 30;
            sprintf(logname, "%s/pro.log", logdir);
            break;
        case 104://sys
            time_i = 60;
            sprintf(logname, "%s/sys.log", logdir);
            break;
        case 105://user
            time_i = 60;
            sprintf(logname, "%s/user.log", logdir);
            break;
    }
    
    sprintf(run, "bash %s", src);
    if(type == 101)sprintf(run, "bash %s %f", src, dyaver);
    
    clock_t start, finish;
    start = clock();
    while(1){
        clock_t start, finish;
        start = clock();
   
        for(int i = 0; i < times; i++){
            char buff[400] = {0};
            if(NULL == (fstream = popen(run, "r"))){
                printf("popen error\n");
                exit(1);
            }
            if(type == 101){
                if(NULL != fgets(buffer, sizeof(buffer), fstream)){
                    strcat(buffer, buff);
                }
                if(NULL != fgets(buff, sizeof(buff), fstream)){
                    dyaver = atof(buff);
                }else{
                    dyaver = 0;
                }
            }else{
                while(NULL != fgets(buff, sizeof(buff), fstream)){
                    strcat(buffer, buff);
                }
            }
            
            int len = strlen(buffer);           //find out warning msg 
            char temp[10] = {0}, temp2[10] = {0};
            strncat(temp2, &buffer[len - 5], 4);
            strncat(temp, &buffer[len - 8], 7);
            if(strcmp(temp, "warning") == 0 || strcmp(temp2, "note") == 0){
                char tmpbuf[256] = {0};
                sprintf(tmpbuf, "%d ", type);
                strcat(tmpbuf, buffer);
                send_database(tmpbuf);
            }


            pthread_mutex_lock(&msg->sm_mutex);
            msg->flag++;
            printf("＊ ");
            fflush(stdout);
            if(msg->flag >= 10){
                if(msg->heart_signal == 0 && msg->cnt == 0){
                    msg->flag = 0;
                    pthread_mutex_unlock(&msg->sm_mutex);
                    pthread_cond_signal(&msg->sm_ready);
                    printf("\nSystem Self Test More Than 10, Start Heart Listening\n"); 
                }else if(msg->heart_signal == 1){
                    printf("\nSystem Self Test More Than 10, And Already Hearted Success\n");
                }
            }
            pthread_mutex_unlock(&msg->sm_mutex);
            sleep(time_i);
            pclose(fstream);
            finish = clock();
            double wait_time = (double)(finish - start) / CLOCKS_PER_SEC * 10000;
            if(wait_time > writetime){
                start = clock();
                FILE *fp = fopen(logname, "a+");
                if(NULL == fp){
                    DBG("Error open logfile\n");
                    exit(1);
                }
                fwrite(buffer, 1, strlen(buffer), fp);
                fclose(fp);
                if(get_file_size(logname) > maxsize){
                    printf("Large File Backup.....\n");
                    write_Pi_log(pilog, "Backup %s", logname);
                    backup(logname);
                    remove(logname);
                }
            }
        }
    }
}


bool client_heart(char *ip, int port){
    int sockfd;
    if((sockfd = socket_connect(ip, port)) < 0){
        close(sockfd);
        return false;
    }
    close(sockfd);
    return true;
}

int main(){
    int shmid;
    int heart_listen;
    int port_heart, port_master, port_msg;
    char port_tmp[5] = {0};
    char msg_port[5] = {0};
    get_conf_value(config, "client_port", port_tmp);
    port_heart = atoi(port_tmp);
    get_conf_value(config, "master_port", port_tmp);
    port_master = atoi(port_tmp);
    get_conf_value(config, "master_ip", master_ip);
    get_conf_value(config, "LogDir", logdir);
    get_conf_value(config, "Log_Backup", log_backup);
    get_conf_value(config, "msg_port", msg_port);
    port_msg = atoi(msg_port);


    printf("****Creating Shared Memory****\n");
    if((shmid = shmget(IPC_PRIVATE, sizeof(struct sm_msg), 0666|IPC_CREAT)) == -1){
        printf("Error in shmget: %s\n", strerror(errno));
        write_Pi_log(pilog, "Error In Shmget: %s", strerror(errno));
        return -1;
    }
    
    share_memory = (char *)shmat(shmid, 0, 0);
    if(share_memory == NULL){
        printf("Error in shmat: %s\n", strerror(errno));
        write_Pi_log(pilog, "Error In Shmat: %s", strerror(errno));
        return -1;
    }
    
    msg = (struct sm_msg *)share_memory;
    msg->heart_signal = 0;
    msg->flag = 0;
    msg->cnt = 0;
    msg->heart = 0;

    pthread_mutexattr_init(&m_attr);
    pthread_condattr_init(&c_attr);
    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&c_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&msg->sm_mutex, &m_attr);
    pthread_cond_init(&msg->sm_ready, &c_attr);

    check_connect(master_ip, port_master, 500000);
    int pid;
    pid = fork();
    if(pid < 0){
        printf("fork error\n");
        return -1;
    }

    if(pid != 0){
        printf("**** Father Creating Connect | Port : %d ****\n", port_heart);
        if((heart_listen = socket_create(port_heart)) < 0){
            printf("Error in socket_create!\n");
            write_Pi_log(pilog, "Error In socket_create; Port: %d", port_heart);
            return -1;
        }
        while(1){
            int fd;
            if((fd = accept(heart_listen, NULL, NULL)) < 0){
                printf("accept error on heart_listen\n");
                write_Pi_log(pilog, "Accept Error On heart_listen");
                close(fd);
            }
            close(fd);
        }
    }else{
        int pid1;
        if((pid1 = fork()) < 0){
            printf("fork error pid1!\n");
            return -1;
        }
        if(pid1 == 0){
            while(1){
                pthread_mutex_lock(&msg->sm_mutex);
                printf("**** Grand_Son Waiting For Heart Listening | Port : %d ****\n", port_master);
                pthread_cond_wait(&msg->sm_ready, &msg->sm_mutex);
                printf("**** Grand_Son Start Heart Listening | Port : %d ****\n", port_master);
                pthread_mutex_unlock(&msg->sm_mutex);
                while(1){
                    if(client_heart(master_ip, port_master)){
                        msg->cnt++;
                        printf("\n%d Hearted Success\n", msg->cnt);
                        msg->heart_signal = 1;
                        msg->cnt = 0;
                        break;
                    }else{
                        msg->cnt++;
                        if(msg->cnt > 10){
                            printf("\n%d Hearted Failed, Waiting 10 min\n", msg->cnt); 
                        }else{
                            printf("\n%d Hearted Failed, Waiting %d min\n", msg->cnt, msg->cnt);
                        }
                    }
                    fflush(stdout);
                    if(msg->cnt <= 10){
                        sleep(2 * msg->cnt);
                    }else{
                        sleep(120);
                    }
                }    
            }
        }else{
            int x= 0;
            int pid2;
            printf("****Son Doing Self Test****\n");
            for(int i = 100; i < 106; i++){
                x = i;
                if((pid2 = fork()) < 0){
                    printf("error fork pid2!\n");
                    continue;
                }
                if(pid2 == 0)break;
            }    
            if(pid2 == 0){
                sys_detect(x);
            }else{
                int msg_fd, sock_ctrl;
                if((msg_fd = socket_create(port_msg)) < 0){
                    //printf("Creating msg_fd error: %s\n", strerror(errno));
                    write_Pi_log(pilog, "Creating msg_fd Error: %s", strerror(errno));
                }
                printf("Server Connecting To Send Signal......\n");
                    
                while(1){
                    if((sock_ctrl = accept(msg_fd, NULL, NULL)) < 0){
                        continue;
                    }
                    char signal[10] = {0};
                    while(recv(sock_ctrl, signal, sizeof(signal), 0) > 0){
                        printf("signal: %s\n", signal);
                        data_trans(signal, sock_ctrl);
                    }
                 
                    fflush(stdout);
                    close(sock_ctrl);
                }
            }
        }
    }
    wait(NULL);
    close(heart_listen);
    return 0;
}
                    
