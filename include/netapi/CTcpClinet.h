#include "CINet.h"

typedef unsigned int uint;

#ifndef _DEF_TCP_PORT
#define _DEF_TCP_PORT   (12345)
#endif

#ifndef _DEF_TCP_SERVER_ADDR
#define _DEF_TCP_SERVER_ADDR "192.168.227.130"
#endif

struct buffer_t{
    buffer_t();
    ~buffer_t();
    int sizeLen;
    int contentSize;
    int pos;
    char* buffer;
    void initBuffer();
};

class CTcpClient : public CINet{
public:
    CTcpClient(CINetMed* _m_pCINet);
    ~CTcpClient();
    bool initNet();
    void unInitNet();
    void recvData();
    int sendData(char* _szBuf, int _iSize, long long sock);
    static unsigned beginRecvThread(void* _arg);
    bool isConnect();
    SOCKET m_pSock;
    uint m_iTid;
    uintptr_t m_pHandle;
    buffer_t m_tStatus;
};
