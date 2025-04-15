#pragma once

#include <QString>
#include <QVector>

#define SMART_READ_CMD_LEN 12
#define INQUIRY_CMD_LEN 6
#define SMART_READ_RESP_LEN 512
#define INQUIRY_RESP_LEN 96
#define SENSE_BUFFER_LEN 32

class AsciiView
{
public:
    AsciiView() = default;

    QVector<unsigned char> readSMARTData(const QString& device_path);
    QString hexDump(const QVector<unsigned char> &data);
};
