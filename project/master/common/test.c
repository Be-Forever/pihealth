/*************************************************************************
	> File Name: test.c
	> Author: 
	> Mail: 
	> Created Time: 2019年03月05日 星期二 19时03分03秒
 ************************************************************************/

#include<stdio.h>
#include"common.h"


int main(){
    char *test = "aaaaa";
    char *pi = "./PiMonitor.log";
    write_Pi_log(pi, "%s\n", test);
    return 0;
}
