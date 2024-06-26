#include "CKernel.h"

using namespace std;

void TcpUnitTest(CKernel* pThis){
    const char testcontent[] = "nihao";
    for(int i=0;i<10;i++){
        pThis->TcpNet_sendData(testcontent, strlen(testcontent));
    }

}

// 构造函数
CKernel::CKernel(){}

// 析构函数
CKernel::~CKernel(){
    qDebug() << __func__;
}

void CKernel::kernelRun()
{

    m_pMainDialog = new MainDialog;
    m_pMainDialog->setWindowFlags(Qt::FramelessWindowHint);
    m_pMainDialog->showNormal();
    // 初始化线程池
    createLinkThreadPool();
    // 使用 TCP 连接到服务器
//    createLinkTcpClient();
    // 测试Tcp模块
//    TcpUnitTest(this);
}


// 初始化线程池
void CKernel::createLinkThreadPool(){
    m_pTpool = new CThreadPool();
    m_pTpool->initThreadPool(this, 50, 5, 50000);
    m_pTpool->startThreadPool();
    return;
}

void CKernel::createLinkTcpClient(){
    m_pTcpClient = new TcpNetApi;
    m_pTcpClient->initClient();
    m_pTcpClient->startClient("192.168.150.128", 12345);
    return;
}

void CKernel::TcpNet_sendData(const char *_szBuf, int _iSize)
{
    message_t* mt = new message_t;
    mt->initMessage();
    mt->setMessage(_szBuf, _iSize, 1, 101);
    m_pTcpClient->pushMessage(mt);
}

void CKernel::TcpNet_closeFromServer(){
    m_pTcpClient->closeFromServer();
}
