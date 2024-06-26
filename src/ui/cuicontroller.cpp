#include "cuicontroller.h"

CUiController::CUiController()
{

}

void CUiController::CUiController_init(){
    // 建立MainDialog
        m_pLoginWindow = new MainDialog;
        m_pLoginWindow->showNormal();
}
