#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QDateTime>
#include "tcpnetapi.h"
#include "CKernel.h"
#include "packdef.h"

void buffer_t::initBuffer(){
    nRecvSizeLen = 0;
    nRecvLen = 0;
    size = 0;
    type = false;
    if(message != NULL){
        delete message;
    }
}


TcpNetApi::TcpNetApi() :
    connected(false),
    mSock(INVALID_SOCKET),
    m_pHandle(0),
    m_pHandleSend(0),
    m_pHandleHeart(0),
    m_iTid(0),
    m_iTidSend(0),
    m_iTidHeart(0),
    m_pkernel(NULL),
    sendingHeart(false),
    sendingSuccess(false)
{

}

void TcpNetApi::initClient()
{
    // 加载库文件
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);
}

void TcpNetApi::startClient(const char* ip, short port)
{
    connectServer(ip, port);
    startRecvThread();
    startSendCustomerThread();
    startHeartLoopThread();
}

bool TcpNetApi::connectServer(const char *ip, short port)
{
    int err;
    // 申请套接字
    mSock = socket(AF_INET, SOCK_STREAM, 0);
    if(mSock == INVALID_SOCKET){
        qDebug() << "error[CTcpClient::initNet -> socket()]: " << WSAGetLastError();
        return false;
    }
    // 设置地址端口复用
    setReuseAddr(mSock);
    // 连接套接字
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = inet_addr(ip);
    err = connect(mSock, (sockaddr*)&addr, sizeof addr);
    if(err == SOCKET_ERROR){
        closesocket(mSock);
        qDebug() << "error[CTcpClient::initNet -> connect()]: " << WSAGetLastError();
        return false;
    }
    // 连接成功，将connected置为1
    connected = 1;
    return true;
}

bool TcpNetApi::pushMessage(message_t *mt)
{
    if(!connected){
        // 如果没有连接到服务器，先关闭接收发送消息线程，并关闭socket
        if(TcpNetApi::checkThreadAlive(m_pHandle)){
            // 如果接收线程还存活，等待退出
            if(WaitForSingleObject(m_pHandle, 100) == WAIT_TIMEOUT){
                // 如果超过1秒，线程还未停止，则直接杀死线程
                TerminateThread(m_pHandle, NULL);
            }
        }
        m_pHandle = NULL;
        if(TcpNetApi::checkThreadAlive(m_pHandleSend)){
            // 如果发送消费者线程还存活，等待退出
            if(WaitForSingleObject(m_pHandleSend, 100) == WAIT_TIMEOUT){
                // 如果超时，直接杀死线程
                TerminateThread(m_pHandleSend, NULL);
            }
        }
        m_pHandleSend = NULL;
        closesocket(mSock);
        // 然后尝试重新连接服务器
        if(!connectServer("192.168.150.128", 12345)){
            // 如果还是连接失败，则返回false
            return false;
        }
        // 如果重连成功，重新开启发送接收线程
        startRecvThread();
        startSendCustomerThread();
        // 等待0.1秒初始化线程
        Sleep(100);
    }
    // 如果连接正常，或者重连后正常，则继续添加消息
    mTsq.enqueue(mt);
    return true;
}

unsigned TcpNetApi::sendMessageLoop(void* arg)
{
    TcpNetApi* pThis = (TcpNetApi*)arg;
    while(!pThis->_exit && !pThis->_sendexit){
        while(pThis->mTsq.isEmpty()){
            // 等待非空或者重连
            Sleep(10);
        }
        // 拿取Message
        message_t* mt = pThis->mTsq.dequeue();
        char* union_message = new char[mt->size + sizeof(mt->size) + 1];
        // 复制size（包含type）
        short size = mt->size | ((short)mt->type << ((sizeof(mt->size) * 8 - 1)));
        memcpy(union_message, (char*)&size, sizeof(mt->size));
        // 复制协议
        memcpy(union_message + sizeof(mt->size), \
               (char*)&mt->protocol, sizeof(mt->protocol));
        // 复制内容
        memcpy(union_message + sizeof(mt->size) + sizeof(mt->protocol), \
               mt->content, mt->size - sizeof(mt->protocol));
        // 开始发送
        int sendBytes = 0;
        int sendAllBytes = 0;
        do{
            // 判断是否发送完毕
            if(sendAllBytes < (int)sizeof(mt->size) + mt->size){
                // 没有发送完毕，继续发送
                sendBytes = send(pThis->mSock, union_message + sendAllBytes, \
                                 (int)sizeof(mt->size) + mt->size - sendAllBytes, 0);
                // 判断结果
                if(sendBytes <= 0){
                    // 发生错误, 断开连接
//                    closesocket(pThis->mSock);
                    // 发生错误，将connected置为0
                    int errcode = WSAGetLastError();
                    qDebug() << "recv failed with error: " << errcode;
                    pThis->connected = 0;
                    pThis->_sendexit = 1;
                    break;
                }else{
                    sendAllBytes += sendBytes;
                    // 再次判断是否发送完毕
                    if(sendAllBytes >= (int)sizeof(mt->size) + mt->size){
                        // 首先，释放mt的空间, 自动释放mt中content的空间
                        delete mt;
                        // 发送完毕，则退出发送循环
                        break;
                    }
                    // 继续发送
                }
            }
        }while(true);
    }
    return 1;
}

int TcpNetApi::recvMessage(buffer_t& bt)
{
    int recvBytes = 0;
    do{
        // 判断长度字节接收是否完毕
        if(bt.nRecvSizeLen < (int)sizeof(bt.size)){
            // 开始接收长度字节
            recvBytes = recv(mSock, (char*)&bt.size+bt.nRecvSizeLen, \
                             sizeof(bt.size)-bt.nRecvSizeLen, 0);
            // 判断返回值
            if(recvBytes <= 0){
                // 出现问题
                int errcode = WSAGetLastError();
                qDebug() << "recv failed with error: " << errcode;
                return recvBytes;
            }else{
                bt.nRecvSizeLen += recvBytes;
                // 判断是否能进入接收数据环节
                if(bt.nRecvSizeLen < (int)sizeof(bt.size)){
                    // 不能则重新接收长度字节
                    continue;
                }
                // 如果接收完毕，size小于等于0，直接返回
                if(bt.size <= 0){
                    // 因为没有报错，所以返回0
                    return 0;
                }
                // 否则设置bt结构体
                bt.type = bt.size & 0x8000;
                bt.size = bt.size | 0x7fff;
                // 分配接收数据用的空间
                bt.message = new char[bt.size];
            }
        }

        // 长度字节接受完毕，开始接收数据
        recvBytes = recv(mSock, bt.message + bt.nRecvLen, \
                         bt.size - bt.nRecvLen, 0);
        if(recvBytes <= 0){
            // 出现问题
            int errcode = WSAGetLastError();
            qDebug() << "recv failed with error: " << errcode;
            return recvBytes;
        }else{
            bt.nRecvLen += recvBytes;
            // 再次判断是否接收成功
            if(bt.nRecvLen < bt.size){
                continue;
            }
            // 接收完毕，返回bt结构体
            return bt.size;
        }
    }while(true);
}

void TcpNetApi::startRecvThread()
{
     _recvexit = 0;
    // 开始消息接收
    m_pHandle = (HANDLE)_beginthreadex(NULL, 0, &TcpNetApi::recvMessageStatic, this, 0, &m_iTid);
}

void TcpNetApi::startSendCustomerThread()
{
    _sendexit = 0;
    // 开启数据发送队列
    m_pHandleSend = (HANDLE)_beginthreadex(NULL, 0, &TcpNetApi::sendMessageLoop, this, 0, &m_iTidSend);
}

void TcpNetApi::startHeartLoopThread()
{
    // 开启心跳机制
    m_pHandleHeart = (HANDLE)_beginthreadex(NULL, 0, &TcpNetApi::heartAliveLoop, this, 0, &m_iTidHeart);
}

unsigned TcpNetApi::recvMessageStatic(void *arg)
{
    TcpNetApi* pThis = (TcpNetApi*)arg;
    buffer_t bt;
    while(!pThis->_exit && !pThis->_recvexit){
        // 初始化buffer_t
        bt.initBuffer();
        // 开始接收数据
        int err = pThis->recvMessage(bt);
        if(err <= 0){
            // 表示此次接收数据存在错误
            // 断开连接
//            closesocket(pThis->mSock);
            // 接收出错，将connected置为0，并退出接收线程, 然后交给断线重连机制
            pThis->connected = 0;
            pThis->_recvexit = 1;
            continue;
        }
        // 开始处理数据（多线程处理）
        // 多线程需要复制数据
        buffer_t* new_bt = new buffer_t;
        memcpy((char*)new_bt, (char*)&bt, sizeof(buffer_t));
        if(bt.type){
            // 传递给上层CLogic进行处理
//            pThis->m_pkernel->dealMessage(new_bt);
            qDebug() << "交由上层dealer处理" << endl;
        }else{
            // 底层直接处理
//            pThis->dealMessage(new_bt);
            qDebug() << "交由底层dealer处理" << endl;
        }
    }
    return 1;
}

unsigned TcpNetApi::heartAliveLoop(void *arg)
{
    TcpNetApi* pThis = (TcpNetApi*)arg;
    qint64 sendHeartTime = 0;
    qint64 reconnectTime = 0;
    while(!pThis->_exit){
        // 休眠1秒
        Sleep(1000);
        // 获取当前时间
        qint64 qtime = QDateTime::currentSecsSinceEpoch();
        // 构造心跳包
        message_t* mt = new message_t;
        QJsonObject mainProtocol;
        mainProtocol["TimeStamp"] = QJsonValue(qtime);
        QJsonDocument jsonDocument = QJsonDocument(mainProtocol);
        QString str = jsonDocument.toJson(QJsonDocument::Indented);
        mt->setMessage(str.toStdString().c_str(), strlen(str.toStdString().c_str()), \
                       0, _DEF_SYS_HEART_REQ);
        // 如果断开连接的话
        cout << qtime << ":" << sendHeartTime << ":" << reconnectTime << endl;
        if(checkThreadAlive(pThis->m_pHandle)){
            cout << "recv Alive" << endl;
        }else{
            cout << "recv killed" << endl;
        }
        if(checkThreadAlive(pThis->m_pHandleSend)){
            cout << "send Alive" << endl;
        }else{
            cout << "send killed" << endl;
        }
        if(!pThis->connected){
            // 获取当前时间
            // 如果距离上一次尝试连接发送超过3秒
            if(qtime - reconnectTime >= 3){
                // 通过发送心跳包，尝试再次进行连接
                pThis->pushMessage(mt); // 如果连接成功，则会将connected置为1
                // 设置reconnectTime和sendHeartTime
                reconnectTime = qtime;
                sendHeartTime = qtime;
                cout << "[" << reconnectTime << "]" << " trying reconnecting server..." << endl;
            }
            continue;
        }
        // 如果连接正常
        if(qtime - sendHeartTime >= 10){
            // 如果距离上次发送心跳包超过10秒，再次发送心跳包
            pThis->pushMessage(mt);
            // 设置sendHeartTime;
            sendHeartTime = qtime;
            cout << "["<< sendHeartTime <<"]" << " sending a heart beat" << endl;
        }
    }
    return 1;
}

bool TcpNetApi::getConnected()
{
    return connected;
}

void TcpNetApi::setReuseAddr(SOCKET sock)
{
    int value = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof value);
}

void TcpNetApi::closeFromServer()
{
    closesocket(mSock);
}

bool TcpNetApi::checkThreadAlive(HANDLE handle)
{
    if(handle){
        DWORD errcode;
        if(GetExitCodeThread(handle, &errcode)){
            return true;
        }
    }
    return false;
}

message_t::message_t():type(false), size(0), protocol(0), content(NULL){}

message_t::~message_t()
{
    initMessage();
}

void message_t::initMessage()
{
    type = false;
    if(content)
        delete[] content;
    size = 0;
    protocol = 0;
}

void message_t::setMessage(const char *_content, int _contentsize, bool _type, short _protocol)
{
    content = new char[_contentsize + 1];
    type = _type;
    protocol = _protocol;
    size = _contentsize + sizeof(protocol);
    // 复制内容
    memcpy(content, _content, _contentsize);
}
