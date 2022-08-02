#ifndef C_LANGUAGE_THREADPOOL_20111209_WANGFEI_H
#define C_LANGUAGE_THREADPOOL_20111209_WANGFEI_H

#include "thpool.h"
#include "queue.h"

typedef struct task_thread_info ttinfo;
typedef struct threadpool  thpool;
typedef struct threads_info thsinfo;


//任务分配线程结构
typedef struct task_thread_info
{
	pthread_t id;                  //线程的id
	queue  queueid;                //任务队列id
	pthread_mutex_t lock;          //线程锁
	pthread_cond_t cond;           //线程条件变量
	thpool* pool;                  //线程池指针
}task_thread_info;

//线程的结构
typedef struct threads_info
{
	int state;
	pthread_t id;                  //线程的id
	char type;                     //工作状态(0空闲，1繁忙)
	procfun workfun;               //工作线程的回调
	pthparm funparm;               //工作线程的回调任务参数
	pthread_mutex_t lock;          //线程锁
	pthread_cond_t  cond;          //线程消息
	thpool* pool;                  //线程池指针
	time_t time;                   //最近一次分配线程工作时间
}threads_info;

//线程池的结构
typedef struct threadpool
{
	unsigned int minnum;           //最小线程池数
	unsigned int curnum;           //当前工作线程数
	unsigned int maxnum;           //最大线程池数
	char type;                     //是否结束的标志
	thsinfo* threads;              //线程池数据
	pthread_mutex_t lock;          //线程锁
	pthread_t manageid;            //管理线程id
	ttinfo taskthread;             //任务分配线程
	queue queueid;                 //线程队列
}threadpool;


// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          创建线程池，创建管理线程、任务分配线程、工作线程
/// @Param pthp--线程池值
///
/// @Retunrs 
//       正常
// ///////////////////////////////////////////////////////////////////////////
int thp_create(thpool* pthp);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           工作线程回调函数
/// @Param arg--线程池结构指针
///
/// @Retunrs 
// ///////////////////////////////////////////////////////////////////////////
static void* workthread(void* arg);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          任务分配线程，首先等待处理消息，取队列中的数据处理
/// @Param arg--线程池值
///
/// @Retunrs 
// ///////////////////////////////////////////////////////////////////////////
static void* taskthread(void* arg);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          管理线程，当空闲线程超过工作线程数时，清空部分空闲线程
/// @Param arg--线程池值
///
/// @Retunrs 
// ///////////////////////////////////////////////////////////////////////////
static void* managethread(void* arg);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          获取线程池的状态
/// @Param pthp--线程池值
///
/// @Retunrs
//          0:空闲数大于繁忙数
//          1：空闲数小于繁忙数
// ///////////////////////////////////////////////////////////////////////////
static int threadpoolstatus(thpool* pthp);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          清空线程池中空闲线程        
/// @Param pthp--线程池值
///
/// @Retunrs 
//         正常
// ///////////////////////////////////////////////////////////////////////////
static int popthread(thpool* pthp);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          在线程池中增加一个工作的线程
/// @Param pthp--线程池值
///
/// @Retunrs 
//         返回一个增加的工作线程数句柄，NULL表示失败
// ///////////////////////////////////////////////////////////////////////////
static thsinfo* pushthread(thpool* pthp);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           赋值处理工作，启动处理
/// @Param pth--线程结构值
/// @Param callback--处理工作的回调
/// @Param parm--回调的参数
///
/// @Retunrs 
//          正常
// ///////////////////////////////////////////////////////////////////////////
static int processjob(thsinfo* pth, procfun callback, pthparm parm);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           对线程池添加工作线程处理工作，当前工作线程如果没有空闲就要添加工作线程
/// @Param pthp--线程池值
/// @Param callback--处理工作的回调
/// @Param parm--回调的参数
///
/// @Retunrs 
//         正常
// ///////////////////////////////////////////////////////////////////////////
static int processwork(thpool* pthp, procfun callback, pthparm parm);

#endif//C_LANGUAGE_THREADPOOL_20111209_WANGFEI_H
