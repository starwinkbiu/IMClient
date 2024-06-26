#ifndef TCPNETAPI_H
#define TCPNETAPI_H

#include <winsock2.h>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include "CThreadPool.h"
#include "ThreadSafeQueue.h"
#include "netMessage.h"

class CKernel;

typedef struct buffer_t{
    int nRecvSizeLen; // 表示前两个字节接收长度
    int nRecvLen; // 表示数据接收总长度
    short size; // 表示数据的长度
    bool type; // 表示二进制数据还是协议数据
    char* message; // 堆指针，存放接收的消息
    void initBuffer();
} buffer_t;

class TcpNetApi
{
public:
    TcpNetApi();
    void initClient();
    void startClient(const char* ip, short port);
    bool connectServer(const char* ip, short port);
    bool pushMessage(message_t* mt);
    // 使用队列进行数据发送，实现异步
    static unsigned sendMessageLoop(void* arg);
    int recvMessage(buffer_t& bt);
    void startRecvThread();
    void startSendCustomerThread();
    void startHeartLoopThread();
    static unsigned recvMessageStatic(void* arg);
    static unsigned heartAliveLoop(void* arg);
    bool getConnected();
    static void setReuseAddr(SOCKET sock);
    void closeFromServer();
    static bool checkThreadAlive(HANDLE handle);

private:
    bool connected;
    SOCKET mSock;
    HANDLE m_pHandle;
    HANDLE m_pHandleSend;
    HANDLE m_pHandleHeart;
    unsigned int m_iTid;
    unsigned int m_iTidSend;
    unsigned int m_iTidHeart;
    bool _exit = false;
    bool _recvexit = false;
    bool _sendexit = false;
    CKernel* m_pkernel;
    ThreadSafeQueue<message_t*> mTsq;
//    QQueue<message_t*> mTsq;
    bool sendingHeart;
    bool sendingSuccess;
};

#endif // TCPNETAPI_H
