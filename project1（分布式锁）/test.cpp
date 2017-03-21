#include <cstdio>
#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include "lock.h"

char filename[256] = "test.txt";

int child_process(int cnt  = 10 )
{
    FILE *fp = NULL;
    int num = 0;
    int pid = getpid();
    
    Dlock lk;
    int n;

    while (cnt -- > 0) {
        while (lk.TryLock(filename) != true) {
           // sleep(1);
        }

        //read
        fp = fopen(filename, "r");
        fscanf(fp, "%d", &num);

        // count
        ++ num;

        //write
        fp = freopen(filename, "w", fp);
        fprintf(fp, "%d", num);
        fclose(fp);

        //log
        printf("child %d: put %d\n", pid, num);

        while (lk.TryUnlock(filename) != true) {
            sleep(1);
            printf("fail to tryunlock\n");
        }
    }
}

int main(int argc, char *argv[])
{
    
    int parent = getpid();
    int nr_child = 3;
    int num;
    FILE *fp = NULL;
    int cnt = 10;

    int opt;

    while ((opt = getopt(argc, argv, "c:n:"))!= -1 ) {
        switch (opt) {
            case 'c':
                nr_child = atoi(optarg);
                break;
            case 'n':
                cnt = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-c nr_child] [-n count]\n \
                        the result should be nr_child * count\n",
                        argv[0]);

                return 0;
        }
    }

    fp = fopen(filename, "w");
    fprintf(fp, "%d", 0);
    fclose(fp);

    for (int i = 0; i < nr_child; ++ i)
        if (parent == getpid() && fork() == 0) 
            child_process(cnt);
    
    if (parent == getpid()) {

        for (int i = 0; i < nr_child; ++ i)
            wait(NULL);

        fp = fopen(filename, "r");
        fscanf(fp, "%d", &num);
        fclose(fp);
        
        printf("check this number(%d) is right!\n", num);
    }

    return 0;
}
