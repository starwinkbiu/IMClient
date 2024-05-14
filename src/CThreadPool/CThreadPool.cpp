#include "CThreadPool.h"
#include <process.h>
#include <windows.h>
#include <iostream>
#include <unistd.h>
#include <QDebug>


using namespace std;

CThreadPool* CThreadPool::G_THREADPOOL;

threadinfo::threadinfo() {
    threadRun = 1;
    threadMax = _DEF_THREADPOOL_THREAD_MAX;
    threadMin = _DEF_THREADPOOL_THREAD_MIN;
    threadAlive = 0;
    threadBusy = 0;
    threadKill = 0;
    mid.handle = 0;
    mid.tid = 0;
    tids = (thread_t*)malloc(threadMax * sizeof(thread_t));
    memset(tids, 0, threadMax * sizeof(thread_t));
}

threadinfo::~threadinfo()
{
    qDebug() << __func__;
    // 变量置 0
    threadRun = 0;
    // 等待线程结束
    for(int i=0; i<threadMax; i++){
        if(tids[i].handle){
            if(WaitForSingleObject((HANDLE)tids[i].handle, 1000)){
                TerminateThread((HANDLE)tids[i].handle, -1);
            }
            CloseHandle((HANDLE)tids[i].handle);
        }
    }
    // 等待管理者线程退出
    // 释放 tids 申请的空间
    free(tids);
    threadMax = 0;
    threadMin = 0;
    threadAlive = 0;
    threadBusy = 0;
    threadKill = 0;
    mid.handle = 0;
    mid.tid = 0;
}

bool threadinfo::running(){
    return threadRun;
}

void threadinfo::initThreadInfo(int _max, int _min){
    threadMax = _max;
    threadMin = _min;
    tids = (thread_t*)realloc(tids, threadMax * sizeof(thread_t));
    memset(tids, 0, threadMax * sizeof(thread_t));
}

void threadinfo::initThread(CThreadPool* _threadManager){
    // 初始化线程
    for(int i=0; i<threadMin; i++){
        create_thread_arg_t* arg = new create_thread_arg_t;
        arg->object = _threadManager;
        arg->idx = i;
        tids[i].handle = _beginthreadex(NULL, 0, _threadManager->customer, arg, 0, &tids[i].tid);
        if(!tids[i].handle){
            cout << __func__ << " error: ";
            cout << "thread create error" << endl;
            return;
        }else{
            threadAlive += 1;
        }
    }
}

void threadinfo::initManager(CThreadPool* _threadManager){
    mid.handle = _beginthreadex(NULL, 0, _threadManager->manager, _threadManager, 0, &mid.tid);
    if(!mid.handle){
        cout << __func__ << " error: ";
        cout << "manager create error" << endl;
        return;
    }
}

taskqueue::taskqueue(){
    rear = 0;
    front = 0;
    max = _DEF_THREADPOOL_TASK_MAX;
    cur = 0;
    tasks = (task_t*)malloc(max * sizeof(task_t));
    memset(tasks, 0, max * sizeof(task_t));
}

void taskqueue::initTaskQueue(int _max){
    max = _max;
    tasks = (task_t*)realloc(tasks, max * sizeof(task_t));
    memset(tasks, 0, max * sizeof(task_t));
}

taskqueue::~taskqueue() {
    // 释放 tasks 空间
    qDebug() << __func__;
    memset(tasks, 0, max * sizeof(task_t));
    free(tasks);
    rear = 0;
    front = 0;
    cur = 0;
    max = 0;
}

bool taskqueue::addTask(task_t task) {
    if(cur < max){
        tasks[front] = task;
        front = (front + 1) % max;
        cur++;
        return 0;
    }
    return 1;
}

int taskqueue::getTask(task_t& task)
{
    if(cur > 0){
        task = tasks[rear];
        rear = (rear + 1) % max;
        cur--;
        return 0;
    }
    return 1;
}

bool taskqueue::empty() {
    return cur == 0;
}

int taskqueue::count() {
    return cur;
}

bool taskqueue::full() {
    return cur == max;
}

// 初始化线程池
CThreadPool::CThreadPool()
{

    // 全局静态线程池指针
    G_THREADPOOL = this;
}

// 销毁线程池
CThreadPool::~CThreadPool()
{
    qDebug() << __func__;
    // 清理静态线程池变量
    G_THREADPOOL = NULL;
    // kernel = 0
    m_pKernel = 0;
}

// 初始化线程池
void CThreadPool::initThreadPool(CKernel* kernel, int _threadMax, int _threadMin, int _max){
    if(_threadMax < _threadMin){
        cout << __func__ << " error: ";
        cout << "_threadMax must bigger than _threadMin" << endl;
        return;
    }
    // 初始化 kernel
    m_pKernel = kernel;
    // 设置线程最大最小值
    pool.initThreadInfo(_threadMax, _threadMin);
    // 设置任务上限
    tq.initTaskQueue(_max);
}

// 开启线程池
void CThreadPool::startThreadPool(){
    pool.initThread(this);
    pool.initManager(this);
}

// 向线程池里添加任务
void CThreadPool::addTask(task_t task){
    if(pool.running()){
        // 申请任务队列锁
        Qlock_task.lock();
        // 判断条件
        while(tq.full()){
            Qcond_task_not_empty.wait(&Qlock_task);
            if(!pool.running()){
                cout << __func__ << " success: ";
                cout << "exit" << endl;
                Qlock_task.unlock();
                return;
            }
        }
        // 添加任务
        tq.addTask(task);
        // 解锁
        Qlock_task.unlock();
        // 通知消费者有任务可以拿了
        Qcond_task_not_empty.notify_one();
    }
}

// 消费者循环拿取任务 static (完成)
unsigned CThreadPool::customer(void* _arg){
    create_thread_arg_t* arg = (create_thread_arg_t*)_arg;
    CThreadPool* pThis = (CThreadPool*)arg->object;
    int idx = arg->idx;
    // 判断线程池是否在运行
    while(pThis->pool.running()){
        // 申请任务队列锁
        pThis->Qlock_task.lock();
//        qDebug() << "[" << GetCurrentThreadId() << "]" << " running.";
        while(pThis->tq.empty()){
            pThis->Qcond_task_not_empty.wait(&pThis->Qlock_task);
            if(!pThis->pool.running()){
                cout << __func__ << " success: ";
                cout << "customer " << pthread_self() << " exit success" << endl;
                // 释放 _arg 空间
                delete (create_thread_arg_t*)_arg;
                pThis->Qlock_task.unlock();
                return 0;
            }
            if(pThis->pool.threadKill){
                // 更新 tids
                pThis->pool.tids[idx].tid = 0;
                pThis->pool.tids[idx].handle = 0;
                // 更新 threadKill
                pThis->pool.threadKill--;
                // 减少存活线程数
                pThis->Qlock_thread_alive.lock();
                pThis->pool.threadAlive -= 1;
                pThis->Qlock_thread_alive.unlock();
//                cout << "delete [" << arg->idx << "] thread success" << endl;
                // 释放 _arg 空间
                delete (create_thread_arg_t*)_arg;
                // 解锁
                pThis->Qlock_task.unlock();
                // 退出线程
                return 0;
            }
        }
        // 接取任务
        task_t task;
        pThis->tq.getTask(task);
        // 解锁
        pThis->Qlock_task.unlock();
        // 如果threadKill==0,唤醒下一个消费者
        // 如果不为0, 就说明manager正在进行
        pThis->Qcond_task_not_empty.notify_one();
        // 添加Busy线程：添加锁
        pThis->Qlock_thread_busy.lock();
        pThis->pool.threadBusy++;
        pThis->Qlock_thread_busy.unlock();
        // 执行任务
        if(task.bussiness){
            task.bussiness(task.arg);
        }
        // 减少Busy线程：添加锁
        pThis->Qlock_thread_busy.lock();
        pThis->pool.threadBusy--;
        pThis->Qlock_thread_busy.unlock();
    }
    return 0;
}
// 你好
// 管理者
unsigned CThreadPool::manager(void* _arg){
    CThreadPool* pThis = (CThreadPool*)_arg;
    int _taskCount;
    int _busy;
    int _alive;
    int _free;
    int _threadMax = pThis->pool.threadMax;
    int _threadMin = pThis->pool.threadMin;
    int addNum = 0;
    int delNum = 0;
    int i;
    int count = 0;
    int nsec = 100000000;
    int sleepTime = 10;
    while(pThis->pool.running()){
        // 访问队列加锁，获取任务数量
        pThis->Qlock_task.lock();
        _taskCount = pThis->tq.count();
        pThis->Qlock_task.unlock();
        // 获取线程总数和繁忙线程以及空闲线程数量
        pThis->Qlock_thread_busy.lock();
        _busy = pThis->pool.threadBusy;
        pThis->Qlock_thread_busy.unlock();
        pThis->Qlock_thread_alive.lock();
        _alive = pThis->pool.threadAlive;
        pThis->Qlock_thread_alive.unlock();
        _free = _alive - _busy;
//        if(_busy > 0)
//        cout << "busy: [" << _busy << "] alive: [" << _alive << "] _free: [" << _free << "]" << " taskCount: [" << _taskCount << "]" << endl;
        // 添加线程规则
        // 1、当任务队列中的任务数量多余空闲线程的时候
        // 2、当busy线程占总线程百分之66以上的时候
        // 3、当先线程数量必须小于threadmax
        if((_taskCount > _free ||(float)_busy / _alive * 100 > 75) && _alive < _threadMax){
            // 如果添加 thread_min 个线程之后，小于等于thread_max
            if(_alive + _threadMin <= _threadMax){
                addNum = _threadMin;
            }else{
                addNum = _threadMax - _alive;
            }
            // 添加 addNum 个新线程
            for(i=0; i<_threadMax && addNum; i++){
                if(!pThis->pool.tids[i].handle){
                    create_thread_arg_t* arg = new create_thread_arg_t;
                    arg->object = pThis;
                    arg->idx = i;
                    pThis->Qlock_task.lock();
                    pThis->pool.tids[i].handle = _beginthreadex(NULL, 0, pThis->customer, arg, 0, &pThis->pool.tids[i].tid);
                    if(!pThis->pool.tids[i].handle){
                        cout << __func__ << " error: ";
                        cout << "thread create error" << endl;
                        return -1;
                    }else{
                        // 添加存活线程数量
                        pThis->Qlock_thread_alive.lock();
                        pThis->pool.threadAlive += 1;
//                        cout << "add [" << i << "] thread success" << endl;
                        pThis->Qlock_thread_alive.unlock();
                    }
                    pThis->Qlock_task.unlock();
                    addNum--;
                }
            }
            // 添加完成，打印一下现状
//            cout << "busy: [" << _busy << "] alive: [" << _alive << "] _free: [" << _free << "]" << " taskCount: [" << _taskCount << "]" << endl;
            // 如果添加完成，就设置B需要在10秒之内，如果没有再次新增线程的话，才能执行
            count = (int)(1000000000 / nsec * sleepTime);
        }else{
            if(--count <= 0){
                // 删除线程规则
                // 1、busy线程占总线程数量的百分之33以下
                // 2、当前线程数必须大于 threadmin
                // 3、空闲状态的线程必须大于 threadmin（因为删除线程数最大为threadMin，而且能够被唤醒的线程只能是空闲线程）
                if((float)_busy / _alive * 100 < 33 && _alive > _threadMin){
                    // 如果_alive - thread_min 小于 thread_min
                    if(_alive - _threadMin < _threadMin){
                        delNum = _alive - _threadMin;
                    }else{
                        delNum = _threadMin;
                    }
                    pThis->pool.threadKill = delNum;
                    for(int i=0; i<delNum; i++){
                        pThis->Qcond_task_not_empty.notify_one();
                    }
                }
                count = 0;
            }
        }
        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = nsec;
        nanosleep(&ts, NULL);
    }
    return 0;
}

























