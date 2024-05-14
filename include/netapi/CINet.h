#include <winsock2.h>

class CINetMed;

class CINet{
public:
    CINet(CINetMed* _m_pCINet) : m_pCINetMed(_m_pCINet) {};
    ~CINet(){};
    virtual bool initNet() = 0;
    virtual void unInitNet() = 0;
    virtual void recvData() = 0;
    virtual int sendData(char* _szBuf, int _iSize, long long sock);
    CINetMed* m_pCINetMed;
};
