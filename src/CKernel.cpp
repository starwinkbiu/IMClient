#include <iostream>
#include "CKernel.h"
#include "CThreadPool.h"
#include "CTcpClientMed.h"
#include <QDebug>

using namespace std;

// 构造函数
CKernel::CKernel(){
    // 建立MainDialog
    m_pMainDialog = new MainDialog;
    m_pMainDialog->showNormal();
    // 初始化线程池
    initThreadPool();
    // 使用 TCP 连接到服务器
//    connectToTcpServer();
}

// 析构函数
CKernel::~CKernel(){
    qDebug() << __func__;
}


unsigned addd(void* _arg){
    timespec ts;
    ts.tv_sec = 5;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
}

// 初始化线程池
void CKernel::initThreadPool(){
    m_pTpool = new CThreadPool();
    m_pTpool->initThreadPool(this, 50, 5, 50000);
    m_pTpool->startThreadPool();
}

void CKernel::connectToTcpServer(){
    m_pTcpMed = new CTcpClientMed(this);
    // 打开 TCP 网络
    m_pTcpMed->openINet();
    m_pTcpMed->sendData("hello", 5, 0);
}
