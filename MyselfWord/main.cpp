#include "myword.h"

#include <QApplication>
#include <QWindow>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QWindow win;
    win.setIcon(QIcon("./myico.ico"));
    MyWord w;
    w.show();
    return a.exec();
}
