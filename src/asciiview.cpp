#include "asciiview.h"

#include <scsi/sg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <QArrayDataPointer>

QVector<unsigned char> AsciiView::readSMARTData(const QString& device_path) {
    int fd;
    unsigned char smart_read_cmd[SMART_READ_CMD_LEN] = {0xa1, 0x0c, 0x0e, 0xd0, 1, 0, 0x4f, 0xc2, 0, 0xb0, 0, 0};
    unsigned char inquiry_cmd[INQUIRY_CMD_LEN] = {0x12, 0, 0, 0, INQUIRY_RESP_LEN, 0};
    unsigned char smart_read_resp[SMART_READ_RESP_LEN];
    unsigned char inquiry_resp[INQUIRY_RESP_LEN];
    unsigned char sense_buffer[SENSE_BUFFER_LEN];
    sg_io_hdr_t io_hdr;
    QVector<unsigned char> result;

    fd = open(device_path.toStdString().c_str(), O_RDONLY);
    if (fd < 0) {
        return result;
    }

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = SMART_READ_CMD_LEN;
    io_hdr.mx_sb_len = SENSE_BUFFER_LEN;
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = SMART_READ_RESP_LEN;
    io_hdr.dxferp = smart_read_resp;
    io_hdr.cmdp = smart_read_cmd;
    io_hdr.sbp = sense_buffer;
    io_hdr.timeout = 20000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        close(fd);
        return result;
    }

    result.resize(SMART_READ_RESP_LEN);
    memcpy(result.data(), smart_read_resp, SMART_READ_RESP_LEN);

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = INQUIRY_CMD_LEN;
    io_hdr.mx_sb_len = SENSE_BUFFER_LEN;
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = INQUIRY_RESP_LEN;
    io_hdr.dxferp = inquiry_resp;
    io_hdr.cmdp = inquiry_cmd;
    io_hdr.sbp = sense_buffer;
    io_hdr.timeout = 20000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        close(fd);
        return result;
    }

    qsizetype currentSize = result.size();
    result.resize(static_cast<int>(currentSize + INQUIRY_RESP_LEN));
    memcpy(result.data() + currentSize, inquiry_resp, INQUIRY_RESP_LEN);

    close(fd);
    return result;
}

QString AsciiView::hexDump(const QVector<unsigned char> &data) {
    QString result;
    qsizetype dataSize = data.size();
    int offset = 0;

    for (qsizetype i = 0; i < dataSize; i += 16) {
        QString line;
        line += QString("%1  ").arg(offset, 8, 16, QChar('0')).toUpper();
        offset += 16;
        for (qsizetype j = 0; j < 16; ++j) {
            if (i + j < dataSize) {
                line += QString("%1 ")
                .arg(static_cast<unsigned char>(data[static_cast<int>(i + j)]), 2, 16, QChar('0'))
                    .toUpper();
            } else {
                line += "   ";
            }
            if (j == 7) line += " ";
        }

        line += " ";
        for (qsizetype j = 0; j < 16 && i + j < dataSize; ++j) {
            unsigned char c = data[static_cast<int>(i + j)];
            line += (c >= 32 && c <= 126) ? QChar::fromLatin1(static_cast<char>(c)) : QChar('.');
        }
        result += line + "\n";
    }
    return result;
}
