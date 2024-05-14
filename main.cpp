#include "maindialog.h"
#include "CKernel.h"
#include "CThreadPool.h"
#include <QApplication>
#include <QDebug>
#include <iostream>
#include <unistd.h>

#include <string>

#include <fstream>
#include "json.hpp"


using namespace std;
typedef nlohmann::json json;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CKernel kernel = CKernel();
    return a.exec();
}
