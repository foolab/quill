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

#ifndef __QUILL_ERROR_H__
#define __QUILL_ERROR_H__

#include <QMetaType>
#include <QuillImageFilter>

class QString;
class QuillErrorPrivate;

class QuillError
{
 public:
    enum ErrorCode {
        UnspecifiedError = -1,
        NoError = 0,

        FileNotFoundError,
        FileOpenForReadError,
        FileReadError,
        FileFormatUnsupportedError,
        FileCorruptError,

        DirCreateError,
        FileOpenForWriteError,
        FileWriteError,

        FileRemoveError,

        GlobalFileLimitError,
        FilterGeneratorError,

        ImageSizeLimitError //! The image dimensions are too big to be shown.
    };

    enum ErrorSource {
        UnspecifiedErrorSource = -1,
        NoErrorSource = 0,

        ImageFileErrorSource,
        ImageOriginalErrorSource,
        ThumbnailErrorSource,
        TemporaryFileErrorSource,
        EditHistoryErrorSource,
        CrashDumpErrorSource
    };

 public:
    QuillError(ErrorCode errorCode = NoError);
    QuillError(ErrorCode errorCode,
               ErrorSource errorSource,
               const QString &errorData);
    QuillError(const QuillError &quillError);

    ~QuillError();

    ErrorCode errorCode() const;
    void setErrorCode(ErrorCode errorCode);

    ErrorSource errorSource() const;
    void setErrorSource(ErrorSource errorSource);

    QString errorData() const;
    void setErrorData(QString errorData);

    QuillError operator=(const QuillError &quillError);

    static QuillError::ErrorCode
        translateFilterError(QuillImageFilter::ImageFilterError error);

 private:
    QuillErrorPrivate *priv;
};

Q_DECLARE_METATYPE(QuillError)

#endif //__QUILL_ERROR_H__
