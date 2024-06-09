#include <QDebug>
#include <iostream>
#include <QApplication>
#include "CKernel.h"
#include "maindialog.h"


using namespace std;
//typedef nlohmann::json json;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    CKernel* kernel = new CKernel;
    kernel->kernelRun();

    return a.exec();
}
