#include <QMutex>
#include <QWaitCondition>
#include <QSemaphore>

#define _DEF_THREADPOOL_THREAD_MAX              (300)
#define _DEF_THREADPOOL_THREAD_MIN              (10)
#define _DEF_THREADPOOL_TASK_MAX                (50000)

typedef unsigned (*FUNC)(void*);

class CKernel;
class CThreadPool;

// 创建线程时候用的参数结构体
struct create_thread_arg_t{
    int idx;
    void* object;
};

// 任务结构体
struct task_t {
    FUNC bussiness;
    void* arg;
};

struct thread_t{
    uintptr_t handle;
    unsigned int tid;
};

// 线程信息结构体
class threadinfo {
public:
    threadinfo();
    ~threadinfo();
    // 以下变量用于管理者管理管理线程池
    // 管理包含：当线程太多的时候，删掉一些线程，当线程过少的时候，添加线程
    int threadRun;
    int threadMax;
    int threadMin;
    // 添加跟删除线程只由管理者进行，所以不存在多线程竞争 threadAlive 变量
    int threadAlive;
    // 线程执行自己获取的任务之前会先将 threadBusy 加一，所以会存在竞争，需要加锁
    int threadBusy;
    // 自杀线程数
    int threadKill;
    // 消费者数组
    thread_t* tids;
    // 管理者
    thread_t mid;
    // 查看线程是否在运行
    bool running();
    // 初始化线程信息
    void initThreadInfo(int _max, int _min);
    // 根据 threadMin 初始化线程
    void initThread(CThreadPool* _threadManager);
    // 初始化管理者
    void initManager(CThreadPool* _threadManager);
};

// 任务队列结构体
class taskqueue {
public:
    taskqueue();
    ~taskqueue();
    task_t* tasks;
    int rear;
    int front;
    int max;
    int cur;
    // 向队列添加任务
    bool addTask(task_t task);
    // 拿取任务
    int getTask(task_t& task);
    // 返回队列是否为空
    bool empty();
    // 返回队列中元素数量
    int count();
    // 返回队列是否已满
    bool full();
    // 初始化任务队列
    void initTaskQueue(int _max);
};

// 线程池
class CThreadPool{
public:
    // kernel 对象, 非本类所有，析构函数无需考虑
    CKernel* m_pKernel;
    // 全局线程池变量
    static CThreadPool* G_THREADPOOL;
    // 线程池管理参数，其中 threadBusy 为临界值
    threadinfo pool;
    // 任务队列，临界值
    taskqueue tq;
    // 条件变量&锁
    QMutex Qlock_task;
    QMutex Qlock_thread_busy;
    QMutex Qlock_thread_kill;
    QMutex Qlock_thread_alive;
    QWaitCondition Qcond_task_not_empty;
    QWaitCondition Qcond_task_not_full;


    CThreadPool();
    ~CThreadPool();

    // 初始化线程池
    void initThreadPool(CKernel* kernel, int _threadMax, int _threadMin, int _max);
    // 开启线程池
    void startThreadPool();
    // 消费者
    static unsigned customer(void* _arg);
    // 添加任务
    void addTask(task_t task);
    // 添加管理者
    static unsigned manager(void* arg);
};
