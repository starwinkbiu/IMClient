#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainDialog; }
QT_END_NAMESPACE

class MainDialog : public QMainWindow
{
    Q_OBJECT

public:
    MainDialog(QWidget *parent = nullptr);
    ~MainDialog();

private:
    Ui::MainDialog *ui;
};
#endif // MAINDIALOG_H
