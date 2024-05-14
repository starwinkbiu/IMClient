#include "CINetMed.h"

class CTcpClientMed : public CINetMed{
public:
    CTcpClientMed(CKernel* _kernel);
    ~CTcpClientMed();
    void openINet();
    void closeINet();
    void sendData(char* _szBuf, int _iSize, long long _tsock);
    void dealData(char* _szBuf, int _iSize, long long _fsock);
};
