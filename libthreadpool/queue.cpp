//////////////////////////////////////////////////////////////////////////////////////
/// @file queue.c
/// @Introduce 
///          c语言版的多线程队列
/// @author Wang Fei
/// @version 1.0
/// @date 2011-12-09
/////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"


// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           初始化队列
/// @Param qhandle--队列值
// ///////////////////////////////////////////////////////////////////////////
void initqueue(queue* phandle)
{
	pthread_mutex_init(&phandle->lock, NULL);
	clearqueue(phandle);
	phandle->front = phandle->back = NULL;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          清空队列
/// @Param qhandle--队列值
// ///////////////////////////////////////////////////////////////////////////
void clearqueue(queue* phandle)
{
	pthread_mutex_lock(&phandle->lock);
	node* pqnode = phandle->front;
	
	while(pqnode != NULL)
	{
		phandle->front = phandle->front->next;
		free(pqnode);
		pqnode = phandle->front;
	}

	phandle->back = NULL;
	phandle->size = 0;
	pthread_mutex_unlock(&phandle->lock);
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          将数据推进队列
/// @Param phandle--队列值
/// @Param value--需要推进数据值
///
/// @Retunrs
//          正常
// ///////////////////////////////////////////////////////////////////////////
int pushqueue(queue* phandle, qtype value)
{
	node* pnew;
	if(NULL == (pnew = (qnode*)malloc(sizeof(qnode))))
	{
		perror("pushqueue malloc error.");
		return -1;
	}

	pnew->data = value;
	pnew->next = NULL;

	pthread_mutex_lock(&phandle->lock);
	if(NULL == phandle->back)//第一次插入数据
		phandle->front = phandle->back = pnew;
	else//插入数据
		phandle->back = phandle->back->next = pnew;

	phandle->size++;
	pthread_mutex_unlock(&phandle->lock);

	return 0;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///         数据推出队列
/// @Param phandle--队列值
///
/// @Retunrs 
//         数据值
// ///////////////////////////////////////////////////////////////////////////
qtype popqueue(queue* phandle)
{
	node* pnew = NULL;
	qtype val = NULL;

	pthread_mutex_lock(&phandle->lock);
	if(NULL == phandle->front)//队列为空
	{
		pthread_mutex_unlock(&phandle->lock);
		return NULL;
	}

	//取数据、队列指针后移
	val = phandle->front->data;
	pnew = phandle->front;
	phandle->front = pnew->next;
	phandle->size--;
	if(NULL == phandle->front)
		phandle->back = NULL;
	pthread_mutex_unlock(&phandle->lock);

	free(pnew);

	return val;
}

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           队列长度
/// @Param phandle--队列值
///
/// @Retunrs --当前队列长度
// ///////////////////////////////////////////////////////////////////////////
int lengthqueue(queue* phandle)
{
	return phandle->size;
}
