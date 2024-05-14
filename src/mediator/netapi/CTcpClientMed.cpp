#include "CTcpClientMed.h"
#include "CTcpClinet.h"
#include "CKernel.h"
#include <QDebug>


CTcpClientMed::CTcpClientMed(CKernel* _kernel) : CINetMed(_kernel){
    // 申请 CTcpClient 对象
    m_pCINet = new CTcpClient(this);
}

CTcpClientMed::~CTcpClientMed(){

}

void CTcpClientMed::openINet(){
    m_pCINet->initNet();
}

void CTcpClientMed::closeINet(){
    m_pCINet->unInitNet();
}

void CTcpClientMed::sendData(char* _szBuf, int _iSize, long long _tsock){
    CTcpClient* tmpCTcpClient = dynamic_cast<CTcpClient*>(m_pCINet);
    if(!tmpCTcpClient->isConnect()){
        // 如果连接断开，则卸载网络，并重新初始化网络
        closeINet();
        // 打开网络
        openINet();
    }
    // 开始发送数据
    if(m_pCINet->sendData(_szBuf, _iSize, _tsock) <= 0){
        // 说明发送数据错误，尝试重新打开网络并重新发送
        // 如果连接断开，则卸载网络，并重新初始化网络
        closeINet();
        // 打开网络
        openINet();
        // 尝试再一次发送
        if(m_pCINet->sendData(_szBuf, _iSize, _tsock) <= 0){
            qDebug() << "error [CTcpClientMed::sendData -> sendData()]";
        }
    }
}

void CTcpClientMed::dealData(char* _szBuf, int _iSize, long long _fsock){
    // 检测长度不为0
    // 发射信号处理消息
    if(_iSize > 0){
        qDebug() << _szBuf;
    }
}

