#ifndef __EMQ_H__
#define __EMQ_H__




#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif

#define FIFO_DEPTH_MAX 0x80     //8*16


typedef struct emq_msg_s {
    int cmd;
    int src;
    int seqNum;
    int length;
    int data[256];
} emq_msg_t;


typedef struct {
    //pthread_mutex_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    pthread_spinlock_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    volatile int head __attribute__ ((aligned(CACHELINE_SIZE)));
    volatile int tail __attribute__ ((aligned(CACHELINE_SIZE)));
    msg_t event[FIFO_DEPTH_MAX+ 2] __attribute__ ((aligned(CACHELINE_SIZE)));
    int depth;
    int size;
    int write_cnt;
    int read_cnt;
    char name[32];

} emq_t __attribute__ ((aligned(CACHELINE_SIZE)));



extern int      workq_init(workq_t *p_q, int size, char *p_name);
extern int      workq_write(workq_t *p_q, msg_t  *p_msg);
extern int      workq_read(workq_t *p_q, msg_t  *p_msg);


#endif
