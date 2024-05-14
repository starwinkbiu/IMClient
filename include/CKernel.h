#include <maindialog.h>

class CThreadPool;
class CTcpClientMed;

class CKernel{
public:
    MainDialog* m_pMainDialog;
    CThreadPool* m_pTpool;
    CTcpClientMed* m_pTcpMed;
    CKernel();
    ~CKernel();
    void initThreadPool();
    void connectToTcpServer();
    void TcpNet_sendData(char* _szBuf, int _iSize, long long sock);
};
