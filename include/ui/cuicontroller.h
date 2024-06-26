#ifndef CUICONTROLLER_H
#define CUICONTROLLER_H
//#include <QObject>
#include "maindialog.h"
#include "CKernel.h"


class CUiController
{
public:
    CUiController();
    void CUiController_init();
    void LoadMainLoginWindow();
    void DeleteMainLoginWindow();
    // 调用CKernel的功能函数
    // 发送消息函数
    void CKernel_SendData(const char* _szBuf, int _iSize);
    // 线程池创建线程
    void CKernel_PushTask();

private:
    CKernel* m_pKernel;
    MainDialog* m_pLoginWindow;
};

#endif // CUICONTROLLER_H
