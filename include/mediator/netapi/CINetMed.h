#include <QObject>

class CKernel;
class CINet;

class CINetMed : public QObject{
    Q_OBJECT;
public:
    CINetMed(CKernel* _kernel);
    ~CINetMed(){};
    virtual void openINet() = 0;
    virtual void closeINet() = 0;
    virtual void sendData(char* _szBuf, int _iSize, long long _tsock) = 0;
    virtual void dealData(char* _szBuf, int _iSize, long long _fsock) = 0;
    CINet* m_pCINet;
    CKernel* kernel;
};
