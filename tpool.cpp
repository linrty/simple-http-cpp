#include "tpool.h"


static void* work_routine(void* args){
   tpool_t* pool = (tpool_t*)args;
   tpool_work_t* work = NULL;
 
   while(1){
        // 对queue_lock上互斥锁
        pthread_mutex_lock(&pool->queue_lock);
        while(!pool->tpool_head && !pool->shutdown){ 
            // 先解除之前对queue_lock上的锁，然后阻塞等待（线程池内没有工作，并且线程池没有shutdown）情况
            // 就会唤醒该函数，再对queue_lock上锁
            pthread_cond_wait(&pool->queue_ready,&pool->queue_lock);
        }

        // 检查线程池的状态
        if(pool->shutdown){
            // 线程池处于shutdown状态，进行解锁
           pthread_mutex_unlock(&pool->queue_lock);
           // 退出线程
           pthread_exit(NULL);
        }
 
        // 取出头节点
        work = pool->tpool_head;
        // head移到下一个节点
        pool->tpool_head = (tpool_work_t*)pool->tpool_head->next;
        pthread_mutex_unlock(&pool->queue_lock);
 
        // 执行取出的节点的函数
        work->work_routine(work->args);
 
        // 释放内存
        free(work);
   }
    return NULL;
}
 
int create_tpool(tpool_t** pool,size_t max_thread_num){
   (*pool) = (tpool_t*)malloc(sizeof(tpool_t));
   if(NULL == *pool){
        // 空间申请失败
        printf("in %s,malloc tpool_t failed!,errno = %d,explain:%s\n",__func__,errno,strerror(errno));
        exit(-1);
   }
   // 初始化线程池状态
   (*pool)->shutdown = 0;
   // 初始化线程池最大线程数
   (*pool)->maxnum_thread = max_thread_num;
   // 先为线程池申请所有线程节点的空间
   (*pool)->thread_id = (pthread_t*)malloc(sizeof(pthread_t)*max_thread_num);
   if((*pool)->thread_id == NULL){
        // 空间申请失败
        printf("in %s,init thread id failed,errno = %d,explain:%s",__func__,errno,strerror(errno));
        exit(-1);
   }
   (*pool)->tpool_head = NULL;
   if(pthread_mutex_init(&((*pool)->queue_lock),NULL) != 0){
        printf("in %s,initial mutex failed,errno = %d,explain:%s",__func__,errno,strerror(errno));
        exit(-1);
   }
   // 初始化一个条件变量  
   if(pthread_cond_init(&((*pool)->queue_ready),NULL) != 0){
        printf("in %s,initial condition variable failed,errno = %d,explain:%s",__func__,errno,strerror(errno));
        exit(-1);
   }
 
   for(int i = 0; i < (int)max_thread_num; i++){
        if(pthread_create(&((*pool)->thread_id[i]),NULL,work_routine,(void*)(*pool)) != 0){
           printf("pthread_create failed!\n");
           exit(-1);
        }
   }
    return 0;
}
 
void destroy_tpool(tpool_t* pool){
   tpool_work_t* tmp_work;
 
   // 检查线程池状态，查看是否已经被销毁  
   if(pool->shutdown){
        return;
   }

   // 修改线程池状态
   pool->shutdown = 1;
 

   pthread_mutex_lock(&pool->queue_lock);
   // 广播，将所有处于阻塞状态的线程唤醒
   pthread_cond_broadcast(&pool->queue_ready);
   pthread_mutex_unlock(&pool->queue_lock);
 
   for(int i = 0; i < (int)(pool->maxnum_thread); i++){
        pthread_join(pool->thread_id[i],NULL);
   }
   free(pool->thread_id);
   while(pool->tpool_head){
        tmp_work = pool->tpool_head;
        pool->tpool_head = (tpool_work_t*)pool->tpool_head->next;
        free(tmp_work);
   }
 
   pthread_mutex_destroy(&pool->queue_lock);
   // 删除条件变量
   pthread_cond_destroy(&pool->queue_ready);
   // 释放线程池信息的空间
   free(pool);
}
 
int add_task_2_tpool(tpool_t* pool,void* (*routine)(void*),void* args){
   tpool_work_t* work,*member;
 
   // 检查一下函数是否为空  
   if(!routine){
        printf("rontine is null!\n");
        return -1;
   }
 
   work = (tpool_work_t*)malloc(sizeof(tpool_work_t));
   if(!work){
        printf("in %s,malloc work error!,errno = %d,explain:%s\n",__func__,errno,strerror(errno));
        return -1;
   }
 
   work->work_routine = routine;
   work->args = args;
   work->next = NULL;
 
   // 需要修改线程池内的成员变量，所以需要上锁操作  
   pthread_mutex_lock(&pool->queue_lock);
   // 链表头插法，插入新的作业
   member = pool->tpool_head;
   if(!member){
        pool->tpool_head = work;
   }
   else{
        while(member->next){
           member = (tpool_work_t*)member->next;
        }
        member->next = work;
   }
 
   // 通知线程池有新的线程加入
   pthread_cond_signal(&pool->queue_ready);
   // 解锁
   pthread_mutex_unlock(&pool->queue_lock);
    return 0;
}
