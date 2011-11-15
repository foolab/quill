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
#include <quillfile.h>

#include <QDir>
#include <QCoreApplication>
#include <QDebug>

#include "utime.h"
#include <sys/types.h>
#include <signal.h>

static const QLatin1String LOCKFILE_SEPARATOR("_");
static const QLatin1String INDEX_SEPARATOR(",");
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

    QString lockfilePrefix = LockFile::lockfilePrefix(quillFile->fileName());

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
    QString lockfilePrefix = LockFile::lockfilePrefix(quillFile->fileName());
    QString lockFilePath = TEMP_PATH
                           + lockfilePrefix
                           + LOCKFILE_SEPARATOR
                           + QString::number(QCoreApplication::applicationPid());

    QFile::remove(lockFilePath);
}

bool LockFile::isQuillFileLocked(const QuillFile* quillFile, bool overrideOwnLock)
{
    QDir tempDir = LockFile::tempDir();
    QString lockfilePrefix = LockFile::lockfilePrefix(quillFile->fileName());

    // check if lock exists for any process
    QLatin1String wildcard("*");
    QStringList nameFilter;
    nameFilter << lockfilePrefix + LOCKFILE_SEPARATOR + wildcard;
    QStringList files = tempDir.entryList(nameFilter, QDir::Files, QDir::NoSort);

    const qint64 ownPid = QCoreApplication::applicationPid();

    Q_FOREACH(QString file, files) {
        QStringList strings = file.split(LOCKFILE_SEPARATOR);

        bool ok;
        qint64 pid = strings.last().toLongLong(&ok);
        if (!ok) {
            // conversion failed
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
            tempDir.remove(file);
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
        int index = file.lastIndexOf(LOCKFILE_SEPARATOR);
        if (index == -1) {
            // failed to parse the lockfile name
            continue;
        }
        file.truncate(index); // truncate _PID part

        // Find indexes for separators
        index = file.lastIndexOf(LOCKFILE_SEPARATOR);
        // Section counting from the end
        QString indexString = file.section(LOCKFILE_SEPARATOR, -1, -1);
        QStringList indexList = indexString.split(INDEX_SEPARATOR);
        // truncate _{comma separated list of indexes}_
        file.truncate(index);

        // Restore separators base on indexes
        foreach(QString index, indexList) {
            bool ok;
            int i = index.toInt(&ok);

            if (!ok) {
                continue;
            }

            file.replace(i, qstrlen(LOCKFILE_SEPARATOR.latin1()), QDir::separator());

        }
        lockedFiles << file;
    }

    return lockedFiles;
}

QString LockFile::lockfilePrefix(const QString& fileName)
{
    // UNIX file system separators cannot be used in filename.
    // Replacing the separator with any valid character leads to collision
    // when the same character is already used in the file name.
    // Thus, mark down the separator indexes in the end of the lockfile name.
    // Format of the lock file:
    // _path_to_file_{comma separated list of separator indexes}_PID
    // For example:
    // /foo/bar__/__image.jpeg
    // _foo_bar_____image.jpg_0,4,10

    QString lockfilePrefix = fileName;
    QList<int> indexes;
    int i = 0;
    while ((i = lockfilePrefix.indexOf(QDir::separator(), i)) != -1) {
        indexes << i;
        ++i;
    }

    lockfilePrefix.replace(QDir::separator(), LOCKFILE_SEPARATOR);
    lockfilePrefix += LOCKFILE_SEPARATOR;

    foreach(int index, indexes) {
        lockfilePrefix += QString::number(index);
        if (index < indexes.last()) {
            lockfilePrefix += INDEX_SEPARATOR;
        }
    }

    return lockfilePrefix;
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
