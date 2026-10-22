#include "utils.h"

#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <iostream>
#include <cstdlib>

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void writeHelper(const QString &path, const QByteArray &name, int exitCode = 0)
{
    QFile file(path);
    check(file.open(QIODevice::WriteOnly), "Cannot create test helper");
    const QByteArray script = "#!/bin/sh\nprintf '" + name +
        "\\n'\nprintf '%s\\n' \"$@\"\nexit " + QByteArray::number(exitCode) + "\n";
    check(file.write(script) == script.size(), "Cannot write test helper");
    check(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner),
          "Cannot make test helper executable");
}

QByteArray runHelper(const QStringList &arguments, int expectedExitCode)
{
    QProcess process;
    utils::startPrivilegedProcess(process, arguments);
    check(process.waitForFinished(5000), "Test helper did not finish");
    check(process.exitStatus() == QProcess::NormalExit, "Test helper crashed");
    check(process.exitCode() == expectedExitCode, "Wrong helper exit code");
    return process.readAllStandardOutput();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    check(directory.isValid(), "Cannot create test directory");
    qputenv("PATH", directory.path().toLocal8Bit());
    const QString pkexec = directory.filePath("pkexec");
    const QString run0 = directory.filePath("run0");
    const QStringList arguments{"/path with spaces/smartctl", "--json=o", "-x", "/dev/example"};
    const QByteArray argumentOutput = arguments.join('\n').toUtf8() + '\n';

    writeHelper(pkexec, "pkexec");
    writeHelper(run0, "run0");
    check(runHelper(arguments, 0) == "pkexec\n" + argumentOutput,
          "pkexec must remain preferred and preserve command arguments");

    writeHelper(pkexec, "pkexec", 126);
    check(runHelper(arguments, 126) == "pkexec\n" + argumentOutput,
          "A rejected pkexec request must not retry with run0");

    check(QFile::remove(pkexec), "Cannot remove test pkexec");
    check(runHelper(arguments, 0) == "run0\n--pipe\n--background=\n--\n" + argumentOutput,
          "run0 must preserve arguments and use direct pipes when pkexec is absent");

    writeHelper(run0, "run0", 1);
    check(runHelper(arguments, 1) == "run0\n--pipe\n--background=\n--\n" + argumentOutput,
          "A rejected run0 request must keep its failure status");

    check(QFile::remove(run0), "Cannot remove test run0");
    QProcess process;
    utils::startPrivilegedProcess(process, arguments);
    check(!process.waitForStarted(5000) && process.error() == QProcess::FailedToStart,
          "Missing privilege helpers must report a launch failure");
}
