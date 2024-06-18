#include "utils.h"

utils::utils() {}

QString utils::getSmartctlPath() {
    QStringList paths = QString::fromLocal8Bit(qgetenv("PATH")).split(QDir::listSeparator(), Qt::SkipEmptyParts);

    paths << "/usr/sbin" << "/usr/local/sbin";

    for (const QString &path : paths) {
        QString absolutePath = QDir(path).absoluteFilePath("smartctl");
        if (QFile::exists(absolutePath) && QFileInfo(absolutePath).isExecutable()) {
            return absolutePath;
        }
    }

    return QString();
}
