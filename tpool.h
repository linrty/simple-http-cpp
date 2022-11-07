#ifndef T_POOL
#define T_POOL

#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <malloc.h>
#include "httpd.h"


/**
 * 线程节点信息
*/
typedef struct tpool_work{
    // 线程中需要执行的函数
    void* (*work_routine)(void*);
    // 参数
    void* args;
    // 指向下一个线程
    struct tpool_work* next;
    /* data */
}tpool_work_t;

/**
 * 线程池相关信息
*/
typedef struct tpool{
    // 线程池状态
    size_t shutdown;
    // 线程池最大线程数
    size_t maxnum_thread;
    // 线程id数组
    pthread_t *thread_id;
    // 线程头节点地址
    tpool_work_t *tpool_head;
    // 条件变量
    pthread_cond_t queue_ready;
    // 锁状态
    pthread_mutex_t queue_lock;
}tpool_t;

/***************************************************
*@brief:
*       创建线程池
*@args:   
*       max_thread_num ---> 线程池最大的线程数
*       pool           ---> 线程池地址
*@return value: 
*       0       ---> 表示创建线程池成功
*       othres  ---> 表示创建线程池失败
***************************************************/
 
int create_tpool(tpool_t** pool,size_t max_thread_num);
 
/***************************************************
*@brief:
*       销毁线程池
*@args:
*        pool  --->  线程池的地址
***************************************************/
void destroy_tpool(tpool_t* pool);
 
/**************************************************
*@brief:
*       add tasks to thread pool
*@args:
*       pool     ---> 线程池地址
*       routine  ---> 线程开始的函数
*       args     ---> 参数
*@return value:
*       0        ---> 表示添加成功
*       others   ---> 表示添加失败        
**************************************************/
int add_task_2_tpool(tpool_t* pool,void* (*routine)(void*),void* args);

#endif//tpool.h
