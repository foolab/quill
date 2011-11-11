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

#ifndef UNIX_PLATFORM_H
#define UNIX_PLATFORM_H

#include <QDateTime>
#include <QString>
#include <QDir>

/* This source file contains UNIX specific implementation parts.
 * Adapt these functions when making Quill portable to e.g. Windows
 */

class QuillFile;

class FileSystem {

 public:
    /*!
      Sets the last-modified datetime for the file. The file must be open.

      @returns true if success, false if failed
    */

    static bool setFileModificationDateTime(const QString &fileName,
                                            const QDateTime &dateTime);
};

class LockFile {
public:
    /*!
      Create a UNIX style lock file for given QuillFile. The lock is process
      specific. If the locking process does not exist anymore, the old lock
      is removed and new lock is created by this process.

      @param quillFile QuillFile to be locked

      @returns true if success, otherwise false
    */
    static bool lockQuillFile(const QString& fileName);

    /*!
      Unlock the given QuillFile

      @param quillFile QuillFile to be locked
     */
    static void unlockQuillFile(const QString& fileName);

    /*!
      @returns true if this QuillFile is locked by a running process,
      otherwise false.
     */

    static bool quillFileLocked(const QString& fileName);

private:
    /*!
      @returns the temporary directory where lock files are created. The directory
      is inside the default system temprorary directory.
    */
    static QDir tempDir();

    /*!
      @returns The prefix part of the lock file name.
      E.g. /foo/bar/image.jpeg --> _foo_bar_image.jpeg_
    */
    static QString lockfilePrefix(const QString& fileName);

    // Allow unit tests to access private utility functions for simulation
    friend class ut_quill;
};

#endif //UNIX_PLATFORM_H
