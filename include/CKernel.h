#ifndef CKERNEL_H
#define CKERNEL_H

#include <maindialog.h>
#include <QDebug>
#include <iostream>
#include "CThreadPool.h"
#include "tcpnetapi.h"

using namespace std;

class CKernel{
public:
    CKernel();
    ~CKernel();
    void kernelRun();
    void createLinkThreadPool();
    void createLinkTcpClient();
    void connectToTcpServer();
    void TcpNet_sendData(const char* _szBuf, int _iSize);
    void TcpNet_closeFromServer();

private:
    MainDialog* m_pMainDialog;
    CThreadPool* m_pTpool;
    TcpNetApi* m_pTcpClient;
};

#endif // CKERNEL_H
