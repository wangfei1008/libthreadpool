#ifndef C_LANGUAGE_QUEUE_20111209_WANGFEI_H
#define C_LANGUAGE_QUEUE_20111209_WANGFEI_H

#include <time.h>
#include <pthread.h>

typedef void* qtype;
typedef struct queuel queue;
typedef struct node qnode;

//节点结构
typedef struct node
{
	qtype data;//数值
	qnode* next;//下来一个指针地址
}node;

//队列结构
typedef struct queuel
{
	node* front;//队列头
	node* back;//队列尾
	unsigned int size;//大小
	pthread_mutex_t lock;//线程锁
}queuel;

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           初始化队列
/// @Param qhandle--队列值
// ///////////////////////////////////////////////////////////////////////////
void initqueue(queue* phandle);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          清空队列
/// @Param qhandle--队列值
// ///////////////////////////////////////////////////////////////////////////
void clearqueue(queue* phandle);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///          将数据推进队列
/// @Param phandle--队列值
/// @Param value--需要推进数据值
///
/// @Retunrs
//          正常
// ///////////////////////////////////////////////////////////////////////////
int pushqueue(queue* phandle, qtype value);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///         数据推出队列
/// @Param phandle--队列值
///
/// @Retunrs 
//         数据值
// ///////////////////////////////////////////////////////////////////////////
qtype popqueue(queue* phandle);

// ///////////////////////////////////////////////////////////////////////////
/// @Introduce 
///           队列长度
/// @Param phandle--队列值
///
/// @Retunrs --当前队列长度
// ///////////////////////////////////////////////////////////////////////////
int lengthqueue(queue* phandle);

#endif//C_LANGUAGE_QUEUE_20111209_WANGFEI_H
