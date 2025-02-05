#include "mainwindow.h"
#include "asciiview.h"

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <iostream>
#include <ostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    bool isHeadless = false;
    QString devicePath;

    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--ascii-view") {
            isHeadless = true;
            if (i + 1 < argc) {
                devicePath = argv[i + 1];
            } else {
                std::cerr << "Error: Missing device path after --ascii-view" << std::endl;
                return 1;
            }
            break;
        }
    }

    if (isHeadless) {
        if (geteuid() == 0) {
            AsciiView asciiview;
            QVector<unsigned char> driveData = asciiview.readSMARTData(devicePath);
            std::cout.write(reinterpret_cast<const char*>(driveData.data()), driveData.size());
            std::cout.flush();
        } else {
            std::cerr << "Error: This action requires root privileges!" << std::endl;
            return 1;
        }
        return 0;
    }

    QApplication a(argc, argv);

    QTranslator translator;
    if (translator.load(QLocale(), QLatin1String("qdiskinfo"), QLatin1String("_"), QLatin1String(":/translations"))) {
        a.installTranslator(&translator);
    }

    MainWindow w;
    w.setWindowIcon(QIcon(":/icons/icon.svg"));
    w.show();
    return a.exec();
}
