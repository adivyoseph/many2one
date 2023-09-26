#define _GNU_SOURCE
#include <assert.h>
#include <sched.h> /* getcpu */
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>   
#include <pthread.h> 
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include "workq.h"

#define IOGENTHREAD_MAX    16
typedef struct ioGenThreadContext_s {
    pthread_t   thread_id;
    int setaffinity;
    workq_t workq_in;
    int id;
} ioGenThreadContext_t;

ioGenThreadContext_t   g_contexts[IOGENTHREAD_MAX + 1];




void usage();
void *th_func(void *p_arg);
void *th_em(void *p_arg);



#define CMD_START 1
#define CMD_STOP    2
#define CMD_CLEAR  3
#define RSP_READY     1
#define RSP_DONE     2

workq_t g_workq_cli;
/**
 * 
 * 
 * @author martin (9/25/23)
 * @brief start
 * @param argc 
 * @param argv 
 * 
 * @return int 
 */
int main(int argc, char **argv) {
    int opt;
    int ioGenThreads = 4;
    int setaffinity  = -1;


    int i, j, k;
    unsigned cpu, numa;
    char work[64];
    char cwork[64];
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */

    msg_t msg;


    for (i = 0; i <= IOGENTHREAD_MAX ; i++) {
        g_contexts[i].setaffinity = -1;
    }

    getcpu(&cpu, &numa);
    printf("CLI %u %u\n", cpu, numa);
      
    while((opt = getopt(argc, argv, "ht:c:s:e:")) != -1) 
    { 
        switch(opt) 
        { 
        case 'h':                   //help
            usage();
            return 0;
            break;

        case 't':                //threads for iogen
                ioGenThreads = atoi(optarg);
                if (ioGenThreads >  IOGENTHREAD_MAX) {
                    ioGenThreads  =  IOGENTHREAD_MAX;
                }
                printf("io_gen_threads: %s\n", optarg);
                break; 

        case 'c':            //cpus for iogen threads
                printf("cpus: %s \n", optarg); 
                //coma seperated list 
                strcpy(cwork, optarg);
                i = 0;
                j = 0;
                k = 0;
                while (cwork[i] != 0) {
                    if (cwork[i] == ' ') {
                        i++;
                        continue;
                    }
                    while ((cwork[i] >= '0') && (cwork[i] <= '9')) {
                        work[j] = cwork[i];
                        work[j+1] = '\0';
                        i++;
                        j++;
                    }
                    if(cwork[i] == '\0'){
                        printf("pin iogen %d to %s\n", k, work);
                        g_contexts[k].setaffinity = atoi(work);
                    }
                    if (cwork[i] == ',') {
                        printf("pin thread %d to %s\n", k, work);
                        g_contexts[k].setaffinity = atoi(work);
                        k++;
                        if (k >=  IOGENTHREAD_MAX) {
                            break;
                        }
                        j = 0;
                        i++;
                    }
                }
                break; 

        case 's':                    //cli cpu mapping
                setaffinity = atoi(optarg);
                printf("cli cpu %d\n", setaffinity); 
                break; 


        case 'e':                    //emulator  cpu mapping
               g_contexts[IOGENTHREAD_MAX].setaffinity = atoi(optarg);
                printf("emulator cpu %d\n", g_contexts[IOGENTHREAD_MAX].setaffinity); 
                break; 
  
        default:
            usage();
            return 0;
                break;

        } 
    } 

    printf("\n");

    CPU_ZERO(&my_set); 
    if (setaffinity >= 0) {
        CPU_SET(setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    getcpu(&cpu, &numa);
    printf("CLI %u %u\n", cpu, numa);



    workq_init(&g_workq_cli, 16, "cli");

    for (i = 0; i <= IOGENTHREAD_MAX; i++) {
        sprintf(&work[0], "wq%d", i);
        workq_init(&g_contexts[i].workq_in, 16, &work[0]);
    }

//    signal(SIGCLD, SIG_IGN);
    for (i = 0;  i < IOGENTHREAD_MAX; i++) {
        g_contexts[i].id = i;
        pthread_create(&g_contexts[i].thread_id, NULL, th_func, (void *) &g_contexts[i]);
    }

    g_contexts[IOGENTHREAD_MAX].id = IOGENTHREAD_MAX;
     pthread_create(&g_contexts[IOGENTHREAD_MAX].thread_id, NULL, th_em, (void *) &g_contexts[IOGENTHREAD_MAX]);

     i = 0;
     while (1) {
            if(workq_read(&g_workq_cli, &msg)){
                if(msg.cmd == RSP_READY) {
                    i++;
                }
             if (i == ioGenThreads) {
                 break;
             }
         }
     }

     printf("all threads ready\n");


    while (1) {
    }



    return 0;
}

void usage(){
    printf("-h     help\n-t     io_gen_threads 1-32\n-c      io_gen cpus x,y,z\n-s    cli_cpu\n-e   emulator cpu\n");
}


/**
 * 
 * 
 * @author martin (9/25/23) 
 *  
 * @brief io gen thread 
 * 
 * @param p_arg 
 * 
 * @return void* 
 */
void *th_func(void *p_arg){
    ioGenThreadContext_t *this = (ioGenThreadContext_t *) p_arg;
    //unsigned cpu, numa;
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
     msg_t msg;
     int send_cnt = 0;

    printf("Thread_%d PID %d %d\n", this->id, getpid(), gettid());


    CPU_ZERO(&my_set); 
    if (this->setaffinity >= 0) {
        CPU_SET(this->setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    msg.cmd = RSP_READY;
    msg.src = this->id;
    msg.length = 0;
    if(workq_write(&g_workq_cli, &msg)){
        printf("%d q is full\n", this->id);
    }

    while (1){
        if(workq_read(&this->workq_in, &msg)){
            switch (msg.cmd) {
            case CMD_START:
                //clear stats
                send_cnt = msg.length;
                break;

            default:
                break;
            }
            if (send_cnt) {
                //send a work item


                send_cnt--;
                if (send_cnt == 0) {
                    msg.cmd = RSP_DONE;
                    msg.src = this->id;
                    msg.length = 0;
                    if(workq_write(&g_workq_cli, &msg)){
                        printf("%d q is full\n", this->id);
                    }
                }
            }
        }
    }
}

 /**
 * 
 * 
 * @author martin (9/25/23) 
 *  
 * @brief emulator thread 
 * 
 * @param p_arg 
 * 
 * @return void* 
 */
void *th_em(void *p_arg){
    ioGenThreadContext_t *this = (ioGenThreadContext_t *) p_arg;
    //unsigned cpu, numa;
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    msg_t msg;

    printf("Emulator  PID %d %d\n", getpid(), gettid());


    CPU_ZERO(&my_set); 
    if (this->setaffinity >= 0) {
        CPU_SET(this->setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }


    while (1){
            if(workq_read(&this->workq_in, &msg)){
            }
    }
}



