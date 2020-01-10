#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> 

typedef void (task_fun)(void *);
struct tasknode_t;
struct threadnode_t;
struct threadpool_t; 

/* 单向链表任务结点 */
typedef struct tasknode_t {
    task_fun            *t_task;            /* 线程工作函数 */
    void                *t_arg;             /* 线程工作函数参数 */
    struct tasknode_t   *t_next;            /* 下一结点指针 */
} tasknode_t;

/* 双向链表线程结点 */
typedef struct threadnode_t {
    pthread_t                t_id;          /* 线程 ID */
    struct threadpool_t     *t_pool;        /* 指向线程池 */
    struct threadnode_t     *t_next;        /* 下一结点指针 */
    struct threadnode_t     *t_prev;        /* 上一结点指针 */
} threadnode_t;

/* 线程池结构 */
typedef struct threadpool_t {
    size_t           tp_min_number;         /* 最小线程数量 */
    size_t           tp_max_number;         /* 最大线程数量 */
    size_t           tp_thread_number;      /* 当前线程数量 */
    size_t           tp_freethread_number;  /* 空闲线程数量 */
    size_t           tp_target_number;      /* 目标线程数量(控制线程数量) */
    tasknode_t      *tp_task_head;          /* 任务链表头部指针 */
    tasknode_t      *tp_task_tail;          /* 任务链表尾部指针 */

    threadnode_t    *tp_freethread_head;    /* 空闲线程节点链表头部指针 */

    pthread_mutex_t  tp_pool_mutex;         /* 线程池互斥量 */
    pthread_mutex_t  tp_task_mutex;         /* 任务链表互斥量 */
    pthread_cond_t   tp_task_not_empty;     /* 任务链表非空条件变量 */
    pthread_cond_t   tp_task_change;        /* 任务链表发送变化条件变量 */
} threadpool_t;


static int task_put(threadpool_t *tp, task_fun *task, void *arg);
static int task_get(threadpool_t *tp, task_fun **task, void **arg);
static void thr_worker_cleanup(void *arg);
static void thr_admin_cleanup(void *arg);
static void *thr_worker(void *arg);
static void *thr_admin(void *arg);
static void thread_leave(threadnode_t *node);
static void thread_join(threadnode_t *node);
static void thread_leave_pool(threadnode_t *node);
static void thread_join_pool(threadnode_t *node);
int threadpool_init(threadpool_t *tp, int min, int max);
int threadpool_destroy(threadpool_t *tp);
int threadpool_insert_task(threadpool_t *tp, task_fun *func, void *arg);


/*
 * 函数说明: 添加任务到线程吃的任务队列中
 * @tp:     线程池地址
 * @task:   需要执行的任务函数
 * @arg:    任务函数需要的参数
 */
static int task_put(threadpool_t *tp, task_fun *task, void *arg)
{
    if (tp == NULL || task == NULL)
        return -1;

    tasknode_t *node;
    if ((node = (tasknode_t *)malloc(sizeof(tasknode_t))) == NULL)
        return -1;

    node->t_arg = arg;
    node->t_task = task;
    node->t_next = NULL;
    pthread_mutex_lock(&tp->tp_task_mutex);
    if (tp->tp_task_tail == NULL)
        tp->tp_task_head = tp->tp_task_tail = node;

    else {
        tp->tp_task_tail->t_next = node;
        tp->tp_task_tail = node;
    }
    pthread_mutex_unlock(&tp->tp_task_mutex);
    pthread_cond_signal(&tp->tp_task_not_empty);
    pthread_cond_signal(&tp->tp_task_change);
    
    return 0;
}


/*
 * 函数说明:    在线程池的任务队列中取任务
 * @tp:         线程池指针
 * @task:       传出任务
 * @arg:        传出任务的参数
 */
static int task_get(threadpool_t *tp, task_fun **task, void **arg)
{
    if (tp == NULL || task == NULL || arg == NULL)
        return -1;

    tasknode_t *node;

    pthread_mutex_lock(&tp->tp_task_mutex);
    if (tp->tp_task_head == NULL)
        pthread_cond_wait(&tp->tp_task_not_empty, &tp->tp_task_mutex);

    /* 清理线程, 执行 线程清理函数 */
    if (tp->tp_task_head == NULL && tp->tp_target_number < tp->tp_thread_number) {
        pthread_mutex_unlock(&tp->tp_task_mutex);
        pthread_exit(NULL);
    }

    node = tp->tp_task_head;
    tp->tp_task_head = tp->tp_task_head->t_next;

    if (tp->tp_task_head == NULL)
        tp->tp_task_tail = NULL;
    
    pthread_mutex_unlock(&tp->tp_task_mutex);
    pthread_cond_signal(&tp->tp_task_change);

    *task = node->t_task;
    *arg = node->t_arg;

    free(node);
    return 0;
}


/*
 * 函数说明:    线程清理函数, 负责清理工作线程, 在线程池 tp_thread_head 链表中的记录
 * @arg:        threadnode_t 类型指针
 */
static void thr_worker_cleanup(void *arg)
{
    threadnode_t *node = (threadnode_t *)arg;
    threadpool_t *tp = node->t_pool;

    pthread_mutex_lock(&tp->tp_pool_mutex);

    --tp->tp_thread_number;
    thread_leave(node);

    pthread_mutex_unlock(&tp->tp_pool_mutex);
    free(arg);
    return;
}


/*
 * 函数说明:    工作线程例程函数, 循环从线程池的任务队列中获取任务, 并执行
 * @arg:        threadnode_t 链表节点参数
 */
static void *thr_worker(void *arg)
{
    threadnode_t *node = (threadnode_t *)arg;
    pthread_detach(pthread_self());
    pthread_cleanup_push(thr_worker_cleanup, arg);

    task_fun *task;
    void *arg;
    while (1) {
        task_get(node->t_pool, &task, &arg);
        thread_leave_pool(node);        
        task(arg);
        thread_join_pool(node);
    }

    pthread_cleanup_pop(1);
    return NULL;
}


/*
 * 函数说明:    线程管理者清理函数, 等待线程池中的所有线程完成任务, 回收线程池资源
 * @arg:        线程池指针
 */
static void thr_admin_cleanup(void *arg)
{
    threadpool_t *tp = (threadpool_t *)arg;

    while (tp->tp_thread_number != 0) {
        pthread_cond_broadcast(&tp->tp_task_not_empty);
        usleep(50);
    }
    
    pthread_mutex_destroy(&tp->tp_pool_mutex);
    pthread_mutex_destroy(&tp->tp_task_mutex);
    pthread_cond_destroy(&tp->tp_task_not_empty);
    pthread_cond_destroy(&tp->tp_task_change);
}


/*
 * 函数功能:    线程池管理者例程函数, 负责管理线程池中的工作线程数量, 当 tp_target_number 为 0 时, 执行线程清理函数
 * @arg:        threadpool_t 线程池地址
 */
static void *thr_admin(void *arg)
{
    threadpool_t *tp = (threadpool_t *)arg;

    pthread_detach(pthread_self());
    pthread_cleanup_push(thr_admin_cleanup, arg);
    
    /* 当 tp_target_number 为 0 时, 退出 while, 调用 pthread_exit 执行线程处理函数, 回收资源 */
    while (tp->tp_target_number != 0) {
        pthread_mutex_lock(&tp->tp_pool_mutex);
        pthread_cond_wait(&tp->tp_task_change, &tp->tp_pool_mutex);

        /* 砍线程 */
        if (tp->tp_freethread_number >= (tp->tp_thread_number / 2)) {
            tp->tp_target_number = 
                (tp->tp_thread_number * 0.70 > tp->tp_min_number ? tp->tp_thread_number * 0.7 : tp->tp_min_number);
            pthread_cond_broadcast(&tp->tp_task_not_empty);
        
        /* 增加线程 */
        } else if (tp->tp_freethread_number < 5 && tp->tp_thread_number < tp->tp_max_number) {
            for (int i = tp->tp_thread_number / 4; i > 0; --i) {
                if (tp->tp_thread_number > tp->tp_max_number)
                    break;

                threadnode_t *node;
                if ((node = (threadnode_t *)malloc(sizeof(threadnode_t))) == NULL)
                    continue;

                node->t_prev = NULL;
                node->t_pool = tp;
                thread_join(node);

                if (pthread_create(&node->t_id, NULL, thr_worker, (void *)node) < 0) {
                    tp->tp_freethread_head = tp->tp_freethread_head->t_next;
                    --tp->tp_freethread_number;
                    free(node);
                }

                ++tp->tp_thread_number;
                ++tp->tp_target_number;
            } // for 增加线程
        } // if (满足增加线程)

        pthread_mutex_unlock(&tp->tp_pool_mutex);
    } // while 

    pthread_exit(NULL);
    pthread_cleanup_pop(1);
}


/* (内部函数)
 * 函数说明:    讲当前节点从线程池的空闲线程链表移除, 减少 tp_freethread_number 计数
 * @node:       节点指针
 */
static void thread_leave(threadnode_t *node)
{
    threadpool_t *tp = node->t_pool;

    if (node->t_prev == NULL) 
        tp->tp_freethread_head = node->t_next;
    else 
        node->t_prev->t_next = node->t_next;
        
    if (node->t_next != NULL)
        node->t_next->t_prev = node->t_prev;

    --tp->tp_freethread_number;
}


/* (内部函数)
 * 函数说明:    讲当前线程节点, 加入到 线程池的 tp_freethread_head 链表中, 增加 tp_freethread_number 的计数 
 * @node:       链表节点
 */
static void thread_join(threadnode_t *node)
{
    threadpool_t *tp = node->t_pool;

    node->t_prev = NULL;
    node->t_next = tp->tp_freethread_head;
    tp->tp_freethread_head = node;
    ++tp->tp_freethread_number;
}


/*
 * 函数功能: 将当前线程节点, 在线程池池的 tp_freethread_head 移除, 减少线程池中, 空闲线程的数量
 * @node:   链表节点
 */
static void thread_leave_pool(threadnode_t *node)
{
    pthread_mutex_lock(&node->t_pool->tp_pool_mutex);
    thread_leave(node);
    pthread_mutex_unlock(&node->t_pool->tp_pool_mutex);
}


/*
 * 函数功能:    将当前节点, 加入到线程池的 tp_freethread_head 链表中
 * @node:       节点指针
 */
static void thread_join_pool(threadnode_t *node)
{
    pthread_mutex_lock(&node->t_pool->tp_pool_mutex);
    thread_join(node);
    pthread_mutex_unlock(&node->t_pool->tp_pool_mutex);
}


/*
 * 函数功能:    初始化线程池
 * @tp:         线程池指针
 * @min:        线程吃的最小线程数量
 * @max:        线程池的最大线程数量
 */
int threadpool_init(threadpool_t *tp, int min, int max)
{
    if (tp == NULL || min > max) 
        return -1;

    tp->tp_freethread_head = NULL;
    tp->tp_task_tail = NULL;
    tp->tp_freethread_head = NULL;
    tp->tp_max_number = max;
    tp->tp_min_number = min;
    tp->tp_thread_number = 0;
    tp->tp_freethread_number = 0;
    tp->tp_target_number = 0;

    if (pthread_mutex_init(&tp->tp_pool_mutex, NULL) < 0)
        return -1;

    if (pthread_mutex_init(&tp->tp_task_mutex, NULL) < 0) {
        pthread_mutex_destroy(&tp->tp_pool_mutex);
        return -1;
    }

    if (pthread_cond_init(&tp->tp_task_not_empty, NULL) < 0) {
        pthread_mutex_destroy(&tp->tp_pool_mutex);
        pthread_mutex_destroy(&tp->tp_task_mutex);
        return -1;
    }

    if (pthread_cond_init(&tp->tp_task_change, NULL) < 0) {
        pthread_mutex_destroy(&tp->tp_pool_mutex);
        pthread_mutex_destroy(&tp->tp_task_mutex);
        pthread_cond_destroy(&tp->tp_task_not_empty);
        return -1;
    }
    
    pthread_mutex_lock(&tp->tp_pool_mutex);
    threadnode_t *node;
    for (int i = 0; i < min; ++i) {
        if ((node = (threadnode_t *)malloc(sizeof(threadnode_t))) == NULL)
            continue;

        node->t_pool = tp;
        thread_join(node);

        if (pthread_create(&node->t_id, NULL, thr_worker, node) < 0) {
            tp->tp_freethread_head = tp->tp_freethread_head->t_next;
            free(node);
            --tp->tp_freethread_number;
            continue;
        }

        ++tp->tp_thread_number;
        ++tp->tp_target_number;
    }

    pthread_t admin;
    while (pthread_create(&admin, NULL, thr_admin, tp) < 0);        /* 循环创建线程, 管理者线程不能失败 */
    pthread_mutex_unlock(&tp->tp_pool_mutex);

    return 0;
}

/*
 * 函数功能:    销毁线程池, 线程池一旦销毁不能使用
 * @tp:         指向线程的指针
 */
int threadpool_destroy(threadpool_t *tp)
{
    if (tp == NULL)
        return -1;

    pthread_mutex_lock(&tp->tp_pool_mutex);
    tp->tp_target_number = 0;
    pthread_mutex_unlock(&tp->tp_pool_mutex);
    pthread_cond_signal(&tp->tp_task_change);

    return 0;
}

/*
 * 函数说明:    添加任务到线程池中
 * @tp:         线程池指针
 * @func:       void (*)(void *) 类型的任务函数
 * @arg:        任务函数的参数
 */
int threadpool_insert_task(threadpool_t *tp, task_fun *func, void *arg)
{
    return task_put(tp, func, arg);
}


void func(void *arg)
{
}

int main(void)
{
    threadpool_t pool;
    if (threadpool_init(&pool, 5, 50) < 0) {
        fprintf(stderr, "threadpool_init error\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < 500; ++i)
        threadpool_insert_task(&pool, func, (void *)i);

    sleep(10);
    threadpool_destroy(&pool);
    sleep(10);

    return 0;
}
