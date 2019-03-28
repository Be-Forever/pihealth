/*************************************************************************
	> File Name: master.h
	> Author: 
	> Mail: 
	> Created Time: 2019年03月09日 星期六 18时24分43秒
 ************************************************************************/

#ifndef _MASTER_H
#define _MASTER_H
#include<stdio.h>
#include <arpa/inet.h>
#include<sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <pthread.h>
#endif
pthread_rwlock_t rwlock;

typedef struct Node{
    int heart_fd;
    int fd;
    struct sockaddr_in cl_addr;
    struct Node *next;
}Node, *LinkedList;


LinkedList init_link(LinkedList head){
    Node *p = (Node *)malloc(sizeof(Node));
    p->cl_addr.sin_family = AF_INET;
    p->cl_addr.sin_port = htons(8731);
    p->cl_addr.sin_addr.s_addr = htons(INADDR_ANY);
    p->next = NULL;
    head = p;
    return head;
}


LinkedList insert(LinkedList head, Node *node){
    Node *current_node;
    if(head == NULL){
        head = node;
        pthread_rwlock_unlock(&rwlock);
        return head;
    }
    current_node = head;
    while(current_node->next != NULL){
        current_node = current_node->next;
    }
    pthread_rwlock_wrlock(&rwlock);
    current_node->next = node;
    pthread_rwlock_unlock(&rwlock);
    return head;
}

LinkedList delete(LinkedList head, char ip[20]){
    Node *current_node = head;
    while(current_node->next != NULL){
        char *p = strdup(inet_ntoa(current_node->next->cl_addr.sin_addr));
        if(strcmp(ip, p) != 0){
            current_node = current_node->next;
            continue;
        }
        break;
    }
    if(current_node->next == NULL){
        pthread_rwlock_unlock(&rwlock);
        return head;
    }
    Node *delete_node = current_node->next;
    pthread_rwlock_wrlock(&rwlock);
    current_node->next = delete_node->next;
    free(delete_node);
    pthread_rwlock_unlock(&rwlock);
    //printf("delete %s success\n", ip);
    return head;
}

void output(LinkedList head){
    pthread_rwlock_rdlock(&rwlock);
    Node *current_node = head;
    while(current_node->next != NULL){
        //printf("ip: %s\n", inet_ntoa(current_node->next->cl_addr.sin_addr));
        current_node = current_node->next;
    }
    pthread_rwlock_unlock(&rwlock);
}


int counter(LinkedList head){
    pthread_rwlock_rdlock(&rwlock);
    Node *current_node = head;
    int cnt = 0;
    while(current_node->next != NULL){
        cnt++;
        current_node = current_node->next;
    }
    pthread_rwlock_unlock(&rwlock);
    return cnt;
}

char *get_ip(LinkedList head, int index){
    Node *current_node = head;
    int n = index;
    while(n){
        current_node = current_node->next;
        n--;
    }
    return inet_ntoa(current_node->cl_addr.sin_addr);
}

int get_fd(LinkedList head, int index){
    Node *current_node = head;
    int n = index;
    while(n){
        current_node = current_node->next;
        n--;
    }
    return current_node->fd;
}

int get_fd_by_ip(LinkedList head, char *ip){
    Node *current_node = head;
    while(current_node->next != NULL && strcmp(ip, inet_ntoa(current_node->cl_addr.sin_addr)) != 0){
        current_node = current_node->next;
    }
    if(current_node->next == NULL){
        return -1;
    }
    return current_node->fd;
}

int check(LinkedList head, in_addr_t check_ip){
    pthread_rwlock_rdlock(&rwlock);
    Node *current_node = head;
    while(current_node->next != NULL){
        if(current_node->next->cl_addr.sin_addr.s_addr == check_ip){
            return 1;
        }
        current_node = current_node->next;
    }
    pthread_rwlock_unlock(&rwlock);
    return 0;
}

void initall(char *from, char *to, LinkedList cl_link){
    struct in_addr addrfrom;
    struct in_addr addrto;
    inet_aton(from, &addrfrom);
    inet_aton(to, &addrto);
    unsigned int nfrom = ntohl(addrfrom.s_addr);
    unsigned int nto = ntohl(addrto.s_addr);
    for(unsigned i = nfrom; i <= nto; i++){
        Node *node = (Node *)malloc(sizeof(Node));
        node->next = NULL;
        node->cl_addr.sin_addr.s_addr = htonl(i);
        insert(cl_link, node);
    }
}



int get_min(LinkedList *head, int index){
    int tmp = 0;
    int cnt = counter(head[0]);
    for(int i = 0; i < index; i++){
        if(counter(head[i]) < cnt){
            tmp = i;
            cnt = counter(head[i]);
        }
    }
    return tmp;
}

