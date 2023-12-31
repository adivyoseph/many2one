#ifndef __WORKQ_H__
#define __WORKQ_H__




#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif

#define FIFO_DEPTH_MAX 0x1f


typedef struct msg_s {
    int cmd;
    int src;
    int seqNum;
    int length;
} msg_t;


typedef struct {
    //pthread_mutex_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    pthread_spinlock_t lock ;//__attribute__ ((aligned(CACHELINE_SIZE)));
    int head ;//__attribute__ ((aligned(CACHELINE_SIZE)));
    int tail ;//__attribute__ ((aligned(CACHELINE_SIZE)));
    msg_t event[FIFO_DEPTH_MAX+ 2] ; //__attribute__ ((aligned(CACHELINE_SIZE)));

} workq_t __attribute__ ((aligned(CACHELINE_SIZE)));



extern int      workq_init(workq_t *p_q);
extern int      workq_write(workq_t *p_q, msg_t  *p_msg);
extern int      workq_read(workq_t *p_q, msg_t  *p_msg);


#endif
