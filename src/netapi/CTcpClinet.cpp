#include "CTcpClinet.h"
#include "CTcpClientMed.h"
#include <QDebug>
#include <process.h>
#include <QMessageBox>


buffer_t::buffer_t() : sizeLen(0), contentSize(0), pos(0), buffer(NULL){

}

buffer_t::~buffer_t(){
}

void buffer_t::initBuffer(){
    sizeLen = 0;
    contentSize = 0;
    pos = 0;
    if(buffer)
        delete buffer;
    buffer = NULL;
}


CTcpClient::CTcpClient(CINetMed* _m_pCINet) : CINet(_m_pCINet), mSock(INVALID_SOCKET), connected(false), m_iTid(0), m_pHandle(0){

}

CTcpClient::~CTcpClient(){
}

bool CTcpClient::initNet(){
    int err;
    // 初始化 winsock2 库
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA data;
    err = WSAStartup(wVersion, &data);
    if(err == SOCKET_ERROR){
        qDebug() << "error[CTcpClient::initNet -> WSAStartup()]: " << WSAGetLastError();
        return err;
    }
    // 连接服务器
    connectServer();
    // 开启循环心跳包检测线程(线程)

    checkAliveLoop();
    return true;
}

void CTcpClient::unInitNet(){
    // 关闭套接字、关闭线程句柄
    // 首先等待线程退出
    if(m_pHandle){
        if(WaitForSingleObject((HANDLE)m_pHandle, 500) == WAIT_OBJECT_0){
            TerminateThread((HANDLE)m_pHandle, -1);
        }
        // 关闭套接字
        if(mSock)
            closesocket(mSock);
    }
    // 初始化接收状态
    m_tStatus.initBuffer();
}


void CTcpClient::connectServer(){
    int err;
    // 申请套接字
    mSock = socket(AF_INET, SOCK_STREAM, 0);
    if(mSock == INVALID_SOCKET){
        qDebug() << "error[CTcpClient::initNet -> socket()]: " << WSAGetLastError();
    }
    // 连接套接字
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_DEF_TCP_PORT);
    addr.sin_addr.S_un.S_addr = inet_addr(_DEF_TCP_SERVER_ADDR);
    err = connect(mSock, (sockaddr*)&addr, sizeof addr);
    if(err == SOCKET_ERROR){
        qDebug() << "error[CTcpClient::initNet -> connect()]: " << WSAGetLastError();
    }
    // 开始消息接收
    m_pHandle = _beginthreadex(NULL, 0, &CTcpClient::beginRecvThread, this, 0, &m_iTid);
}
void CTcpClient::closeServer(){

}
// 心跳机制
bool CTcpClient::heartAlive(){

}
// 循环检测心跳
void CTcpClient::checkAliveLoop(){

}
// 获取connected
bool CTcpClient::getConnected(){

}



void CTcpClient::recvData(){
    // 调用 CTcpClientMed 中介者的 dealData 函数来进行处理
    // 包格式： 4字节长度+内容
    int size = 0;
    while(1){
        // 首先判断是否读取完整
        if(m_tStatus.sizeLen < (int)sizeof(m_tStatus.contentSize)){
            // 如果长度没有读取完整， 首先接收四字节长度
            size = recv( m_pSock, (char*)&m_tStatus.contentSize + m_tStatus.sizeLen, \
                         sizeof(m_tStatus.contentSize) - m_tStatus.sizeLen, 0);
            if(size == -1){
                // 远程服务器出现不可预知错误
                break;
            }else if(size == 0){
                // 服务器关闭了与自己的通话连接
                break;
            }else{
                // 接收正常, 更新 m_tStatus.sizeLen
                m_tStatus.sizeLen += size;
                continue;
            }
        }else{
            // 如果长度读取完整的话, 开始接收数据
            // 首先判断是否是第一次接收
            if(m_tStatus.pos == 0){
                // 如果是第一次接收， 初始化 m_pStatus.buffer
                m_tStatus.buffer = new char[m_tStatus.contentSize];
            }
            size = recv(m_pSock, m_tStatus.buffer + m_tStatus.pos, \
                        m_tStatus.contentSize - m_tStatus.pos, 0);
            if(size == -1){
                // 远程服务器出现不可预知错误
                break;
            }else if(size == 0){
                // 服务器关闭了与自己的通话连接
                break;
            }else{
                // 接收正常, 更新 m_tStatus.sizeLen
                m_tStatus.pos += size;
                continue;
            }
        }
        // 判断内容是否接收完毕
        if(m_tStatus.contentSize == m_tStatus.pos){
            // 将内容发送至 media 的 dealData 函数进行进一步解析处理
            char* tmpContent = new char[m_tStatus.contentSize];
            // 接下来是多线程处理， 并非同步进行， 所以这里不需要释放 tmpContent 空间
            m_pCINetMed->dealData(tmpContent, m_tStatus.contentSize, 0);
            // 重新初始化接收状态
            m_tStatus.initBuffer();
        }
    }
    if(size == 0){
        // 远程服务器正常关闭了与自己的连接
        qDebug() << "remote server close the socket...";
        return;
    }
    int err = WSAGetLastError();
    if(err == WSAECONNRESET){
        // 服务器或者自己突然断开网络
        qDebug() << "error [CTcpClient::recvData]";
    }else if(err == WSAETIMEDOUT ){
        qDebug() << "error [CTcpClient::recvData]";
    }else if(err == WSAENETDOWN ){
        qDebug() << "error [CTcpClient::recvData]";
    }
    // 调用 unInitNet 卸载网络
    unInitNet();
    return;
}

int CTcpClient::sendData(char* _szBuf, int _iSize, long long sock){
    // 启用多线程发送数据（咱为实现）
    // 首先，在要发送的内容前面加上4字节长度
    char* tmpSend = new char[_iSize + 4];
    *(int*)tmpSend = _iSize;
    memcpy(tmpSend+4, _szBuf, _iSize);
    int err = send(m_pSock, tmpSend, _iSize + 4, 0);
    if(err <= 0){
        qDebug() << "error [CTcpClient::sendData -> send()]: " << WSAGetLastError();
    }
    return err;
}

unsigned CTcpClient::beginRecvThread(void* _arg){
    CTcpClient* pThis = (CTcpClient*)_arg;
    // 开始阻塞接收数据
    pThis->recvData();
}

bool CTcpClient::isConnect(){
    // 首先判断套接字是否正常
    int error = 0;
    int len = sizeof(error);
    if (getsockopt(m_pSock, SOL_SOCKET, SO_ERROR, (char*)&error, &len) != SOCKET_ERROR) {
        if (error == 0) {
            return true;  // 没有错误，套接字仍然处于连接状态
        }
    }
    return false;  // 套接字有错误
}









