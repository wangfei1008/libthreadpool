/////////////////////////////////////////////////////////////////////
/// @file threadpool.c
/// @Introduce 
///          线程池,1个线程管理工作线程繁忙(空间)状态，空闲状态超过临界值，降低线程数
///          1个线程分配任务保证工作线程不会超过饱和状态，n个工作线程负责处理工作         
/// @author Wang Fei
/// @version 1.1
/// @date 2011-12-09
/// @rewrite 
///       2012/06/12 根据实际需要修改为独立线程池,主要修改了任务分配线程通用性。
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdio.h>
#include <signal.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <Windows.h>
#include "threadpool.h"
#include "thpool.h"

//主线程休息的时间
#define MANAGE_THREAD_INTERVAL  5000

void bzero(void* src, int num)
{
	memset(src, '\0', num);
}

void defworkfun(pthparm arg)
{
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           初始化线程池，根据线程池的最大数、最小数初始化
/// @Param minnum--最小线程池线程数
/// @Param maxnum--最大线程池线程数
///
/// @Retunrs 
//          线程池值
// ///////////////////////////////////////////////////////////////////////////
pthpool thp_init(unsigned int minnum, unsigned int maxnum)
{
	int size = 0;
	thpool* pth = NULL;
	
	size = sizeof(thpool);
	pth = (thpool*)malloc(size);
	if(NULL == pth)
	{
		logerror("createthreadpool malloc error.\n");
		return NULL;
	}
	bzero(pth, size);

	//初始化成员变量
	pth->curnum = pth->minnum = minnum;
	pth->maxnum = maxnum;
	pth->type = 0;

	pthread_mutex_init(&pth->lock, NULL);

	//分配内存给线程池
	if(NULL != pth->threads)
		free(pth->threads);
	size = sizeof(thsinfo) * maxnum;
	pth->threads = (thsinfo*)malloc(size);
	bzero(pth->threads, size);

	//初始化队列
	initqueue(&pth->queueid);

	//返回创建线程池
	return thp_create(pth) != 0 ? NULL : pth;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          创建线程池，创建管理线程、任务分配线程、工作线程
/// @Param pthp--线程池值
///
/// @Retunrs 
//       正常
// ///////////////////////////////////////////////////////////////////////////
int thp_create(thpool* pthp)
{
	unsigned int i = 0, err = 0;
	thsinfo* pnew = NULL;

	//创建管理线程
	err = pthread_create(&pthp->manageid, NULL, managethread, pthp);
	if(0 != err)
	{
loop:	logerror("createthreadpool create thread error.\n");
		clearqueue(&pthp->queueid);//清空队列
		return -1;
	}

	//创建任务分配线程,初始化任务分配线程
	pthread_mutex_init(&pthp->taskthread.lock, NULL);
	pthread_cond_init(&pthp->taskthread.cond, NULL);
	pthp->taskthread.pool = pthp;
	initqueue(&pthp->taskthread.queueid);
	err = pthread_create(&pthp->taskthread.id, NULL, taskthread, pthp);
	if(0 != err)
		goto loop;

	//创建工作线程
	for( i = 0 ; i < pthp->minnum; i++)
	{
		//变量初始化
		pnew = pthp->threads + i;
		pnew->pool = pthp;
		pnew->type = 0;
		pnew->funparm = 0;
		pnew->workfun = defworkfun;

		//线程琐、信号的初始化
		pthread_mutex_init(&pnew->lock, NULL);
		pthread_cond_init(&pnew->cond, NULL);

		pushqueue(&pthp->queueid, pnew);//把创建的线程数据推进队列

		err = pthread_create(&pnew->id, NULL, workthread, pnew); 
		if(0 != err)
			goto loop;
	}

	return 0;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          关闭线程池，关闭工作线程，管理线程、任务分配线程
/// @Param pthp--线程池值
// ///////////////////////////////////////////////////////////////////////////
void thp_close(pthpool pthpl)
{
	thpool* pthp = (thpool*)pthpl;
	unsigned int i = 0;
	pthp->type = 1;

	//关闭工作线程
	for(i = 0; i < pthp->curnum; i++)
		pthread_cond_signal(&pthp->threads[i].cond);//发送解除阻塞信号
	for(i = 0; i < pthp->curnum; i++)
	{
		pthread_join(pthp->threads[i].id, NULL);
		pthread_mutex_destroy(&pthp->threads[i].lock);
		pthread_cond_destroy(&pthp->threads[i].cond);
	}

	//关闭任务分配线程
	pthread_cond_signal(&pthp->taskthread.cond);
	//kill((pid_t)pthp->taskthread.id, SIGKILL);
	pthread_mutex_destroy(&pthp->taskthread.lock);
	pthread_cond_destroy(&pthp->taskthread.cond);
	clearqueue(&pthp->taskthread.queueid);

	//关闭管理线程
	//kill((pid_t)pthp->manageid, SIGKILL);
	pthread_mutex_destroy(&pthp->lock);

	clearqueue(&pthp->queueid);//清空队列

	//释放线程池结构
	free(pthp->threads);
	pthp->threads = NULL;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           工作线程回调函数
/// @Param arg--线程池结构指针
///
/// @Retunrs 
// ///////////////////////////////////////////////////////////////////////////
static void* workthread(void* arg)
{
	thsinfo* pth = (thsinfo*)arg;

	while(0 == pth->pool->type)//线程池开启状态
	{
		pth->state = 99;
		loginfo("workthread wait\n");
		//等待条件处理工作
		pthread_mutex_lock(&pth->lock);
		pthread_cond_wait(&pth->cond, &pth->lock);
		pthread_mutex_unlock(&pth->lock);
		pth->state = 3;
		loginfo("workthread wait end, start work\n");
		//处理回调
		if(pth->workfun == NULL)
		{
			logerror("workfun error:NULL\n");
			continue;
		}
		pth->workfun(pth->funparm);
		loginfo("workthread work end , to add 1\n");
		pth->state = 4;
		//处理工作完成，重新设置线程状态并推进队列等待下次任务分配
		pth->type = 0;
		pth->time = 0;
		pushqueue(&pth->pool->queueid, (qtype)pth);
		loginfo("add 1 end\n");
	}
	return NULL;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          任务分配线程，首先等待处理消息，取队列中的数据处理
/// @Param arg--线程池值
///
/// @Retunrs 
// ///////////////////////////////////////////////////////////////////////////
static void* taskthread(void* arg)
{
	int size = 0;
	thpool* pthp = (thpool*)arg;
	procfun callback;
       	pthparm parm;

	while(1)
	{
		//等待条件处理工作
		//pthread_mutex_lock(&pthp->taskthread.lock);
		//pthread_cond_wait(&pthp->taskthread.cond, &pthp->taskthread.lock);
		//pthread_mutex_unlock(&pthp->taskthread.lock);

		while(0 < lengthqueue(&pthp->taskthread.queueid))//有任务消息需要处理
		{
			Sleep(10);
			if((pthp->curnum == pthp->maxnum)
		        && (0 == lengthqueue(&pthp->queueid)))//当前工作数目等于需要的最大数，而且没有线程可供分配,需要等待
					continue;
	
			callback = (procfun)popqueue(&pthp->taskthread.queueid);
			parm = (pthparm)popqueue(&pthp->taskthread.queueid);
			processwork(pthp, callback, parm);
		}
		Sleep(100);
	}

	return NULL;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          管理线程，当空闲线程超过工作线程数时，清空部分空闲线程
/// @Param arg--线程池值
///
/// @Retunrs 
// ///////////////////////////////////////////////////////////////////////////
static void* managethread(void* arg)
{
	thpool* pthp = (thpool*)arg;

	Sleep(MANAGE_THREAD_INTERVAL);

	while(0 == pthp->type)//线程池开启标志
	{
	
		//清空超时连接的socket
		//cleanovertimesock(pthp);

		if(0 == threadpoolstatus(pthp))//空闲数过多，需要清空部分
		{
			while(1)
			{
				if(-1 == popthread(pthp))
					break;
			}
		}

		Sleep(MANAGE_THREAD_INTERVAL);
		system("cls");
	}

	return NULL;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          获取线程池的状态
/// @Param pthp--线程池值
///
/// @Retunrs
//          0:空闲数大于繁忙数
//          1：空闲数小于繁忙数
// ///////////////////////////////////////////////////////////////////////////
static int threadpoolstatus(thpool* pthp)
{
	unsigned int i = 0;
	unsigned int busynum = 0;
	thsinfo* pth = NULL;
	for(i = 0; i < pthp->maxnum; i++)
	{
		pth = pthp->threads + i;
		if(pth->state == 2)
		{
			pthread_mutex_lock(&pth->lock);
			pthread_cond_signal(&pth->cond);//启动
			pthread_mutex_unlock(&pth->lock);
		}
	}
	busynum = pthp->curnum - pthp->queueid.size;//当前工作的线程减空闲线程数
	loginfo("busy thread %d, sleep thread: %d\n", busynum, pthp->queueid.size );
	return busynum  > pthp->queueid.size ? 1 : 0;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          清空线程池中空闲线程        
/// @Param pthp--线程池值
///
/// @Retunrs 
//         正常
// ///////////////////////////////////////////////////////////////////////////
static int popthread(thpool* pthp)
{
	thsinfo* pth = NULL;
	thsinfo thinfo;

	if(pthp->curnum <= pthp->minnum)//当前线程数不能小于最小线程数
		return -1;

	if(NULL == (pth = (thsinfo*)popqueue(&pthp->queueid)))//队列中不能没有可清空的队列)
		return -1;

	pthread_mutex_lock(&pthp->lock);
	pthp->curnum -= 1;//队列中清除成功后，当前工作线程数减1

	memcpy(&thinfo, pth, sizeof(thsinfo));
	memcpy(pth, pthp->threads + pthp->curnum, sizeof(thsinfo));
	
	//结束线程，清空结构
	pthread_cond_destroy(&thinfo.cond);
	pthread_mutex_destroy(&thinfo.lock);
	pthread_cancel(thinfo.id);
	pthread_mutex_unlock(&pthp->lock);
	pth->state = 0;

	return 0;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          在线程池中增加一个工作的线程
/// @Param pthp--线程池值
///
/// @Retunrs 
//         返回一个增加的工作线程数句柄，NULL表示失败
// ///////////////////////////////////////////////////////////////////////////
static thsinfo* pushthread(thpool* pthp)
{
	thsinfo* pth = NULL;
	int i = 0;

	if(pthp->maxnum <= pthp->curnum) return NULL;//最大线程数小于当前工作线程数

	//取出初始化好的数据
	while(1)
	{
		pth = pthp->threads + i;
		if(pth->state == 0)
			break;
		i++;
	}
	pth->pool = pthp;

	//初始化琐、条件
	pthread_cond_init(&pth->cond, NULL);
	pthread_mutex_init(&pth->lock, NULL);
	if(0 !=  (pthread_create(&pth->id, NULL, workthread, pth)))
	{
		printf("create pthread fasle\n");
		pthread_mutex_unlock(&pthp->lock);
		return NULL;
	}

	pthp->curnum++;//线程池中当前工作线程数增加1
	pth->state = 200;
	return pth;
}

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
static int processjob(thsinfo* pth, procfun callback, pthparm parm)
{
	if(pth == NULL) return -1;

	loginfo("processjob %d, %d\n", callback, parm );

	//printf("processjob\n");
	//printf("%d\n", pth);
	//printf("%d\n", pth->state);
	//赋值
	pth->type = 1;
	pth->workfun = callback;
	pth->funparm = parm;
	pthread_mutex_lock(&pth->lock);
	pthread_cond_signal(&pth->cond);//启动
	pthread_mutex_unlock(&pth->lock);
	pth->state = 2;
	pth->time = time(NULL);
	Sleep(1);
	return 0;
}

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
static int processwork(thpool* pthp, procfun callback, pthparm parm)
{
	thsinfo* pth = NULL;
	pth = (thsinfo*)popqueue(&pthp->queueid);
	if(-1 == processjob(pth, callback, parm))//当前工作线程全部繁忙
	{
		//添加新的工作线程
		pthread_mutex_lock(&pthp->lock);
		pth = (thsinfo*)pushthread(pthp);
		pthread_mutex_unlock(&pthp->lock);

		Sleep(1000);
		return processjob(pth, callback, parm);
	}

	return 0;
}


// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           添加处理的任务
/// @Param pthp--线程池值
/// @Param callback--处理函数的回调
/// @Param epollhandle--epoll句柄
/// @Param parm--回调的参数
///
/// @Retunrs 
//          正常
// ///////////////////////////////////////////////////////////////////////////
int thp_processtask(pthpool pthp, procfun callback, pthparm parm)
{
	int err = 0;
	thpool* pthpl = (thpool*)pthp;

	//添加任务
	err = pushqueue(&pthpl->taskthread.queueid, callback);
	err = pushqueue(&pthpl->taskthread.queueid, parm);
	pthread_cond_signal(&pthpl->taskthread.cond);
	
	return err;
}
