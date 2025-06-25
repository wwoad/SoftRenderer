#include <QApplication>
#include "Widget.h"
#include "RenderWidget.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
