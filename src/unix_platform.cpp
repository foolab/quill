/****************************************************************************
**
** Copyright (C) 2009-11 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Pekka Marjola <pekka.marjola@nokia.com>
**
** This file is part of the Quill package.
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

#include "unix_platform.h"
#include "quillfile.h"

#include <QDir>
#include <QCoreApplication>
#include <QUrl>

#include <utime.h>
#include <sys/types.h>
#include <signal.h>

static const QLatin1String LOCKFILE_SEPARATOR("_");
static const QString TEMP_PATH = QDir::tempPath()
                                 + QDir::separator()
                                 + "quill"
                                 + QDir::separator();

bool FileSystem::setFileModificationDateTime(const QString &fileName,
                                             const QDateTime &dateTime)
{
    struct utimbuf times;
    times.actime = times.modtime = dateTime.toTime_t();
    int result = utime(fileName.toLocal8Bit().constData(), &times);
    return (result != 0);
}

bool LockFile::lockQuillFile(const QuillFile* quillFile, bool overrideOwnLock)
{
    if (isQuillFileLocked(quillFile, overrideOwnLock)) {
        return false;
    }

    QString lockfilePrefix = QUrl::toPercentEncoding(quillFile->fileName());

    // create the lock file
    QString lockFilePath = TEMP_PATH
                           + lockfilePrefix
                           + LOCKFILE_SEPARATOR
                           + QString::number(QCoreApplication::applicationPid());

    QFile lockFile(lockFilePath);
    if (!lockFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    return true;
}

void LockFile::unlockQuillFile(const QuillFile* quillFile)
{
    QString lockfilePrefix = QUrl::toPercentEncoding(quillFile->fileName());
    QString lockFilePath = TEMP_PATH
                           + lockfilePrefix
                           + LOCKFILE_SEPARATOR
                           + QString::number(QCoreApplication::applicationPid());

    QFile::remove(lockFilePath);
}

bool LockFile::isQuillFileLocked(const QuillFile* quillFile, bool overrideOwnLock)
{
    QDir tempDir = LockFile::tempDir();
    QString lockfilePrefix = QUrl::toPercentEncoding(quillFile->fileName());

    // check if lock exists for any process
    QLatin1String wildcard("*");
    QStringList nameFilter;
    nameFilter << lockfilePrefix + LOCKFILE_SEPARATOR + wildcard;
    QStringList files = tempDir.entryList(nameFilter, QDir::Files, QDir::NoSort);

    const qint64 ownPid = QCoreApplication::applicationPid();

    Q_FOREACH(QString lockFile, files) {
        QStringList strings = lockFile.split(LOCKFILE_SEPARATOR);

        bool ok;
        qint64 pid = strings.last().toLongLong(&ok);
        if (!ok) {
            // conversion failed
            continue;
        }

        // Skip lock for another file
        if (parseLockedfileName(lockFile) != quillFile->fileName()) {
            continue;
        }

        // Ignore own lock if requested
        if (overrideOwnLock && pid == ownPid) {
            continue;
        }

        // Perform error checking without actually signaling to a process
        int value = kill(pid, 0);
        // Process with PID exists, cannot lock
        if (value == 0) {
            return true;
        }
        else {
            // Remove lock with non-existent process
            tempDir.remove(lockFile);
        }
    }

    return false;
}

QStringList LockFile::lockedFiles()
{
    QDir tempDir = LockFile::tempDir();

    QLatin1String wildcard("*");
    QStringList nameFilter;
    // wildcard for name and PID
    nameFilter << wildcard + LOCKFILE_SEPARATOR + wildcard;
    QStringList files = tempDir.entryList(nameFilter, QDir::Files, QDir::NoSort);

    QStringList lockedFiles;

    Q_FOREACH(QString file, files) {
        lockedFiles << parseLockedfileName(file);
    }

    return lockedFiles;
}

QString LockFile::parseLockedfileName(const QString& fileName)
{
    QString percentEncoded = fileName;
    int index = percentEncoded.lastIndexOf(LOCKFILE_SEPARATOR);
    if (index == -1) {
        return QString();
    }

    percentEncoded.truncate(index); // truncate _PID part

    QByteArray array;
    array.append(percentEncoded);
    return QUrl::fromPercentEncoding(array);
}

QDir LockFile::tempDir()
{
    QDir tempDir(TEMP_PATH);
    if (!tempDir.exists()) {
        if (!QDir().mkdir(TEMP_PATH))
            return QDir();
    }

    return tempDir;
}
