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

#include "quillerror.h"

class QuillErrorPrivate {
public:
    QuillError::ErrorCode errorCode;
    QuillError::ErrorSource errorSource;
    QString errorData;
};

QuillError::QuillError(ErrorCode errorCode) :
    priv(new QuillErrorPrivate)
{
    priv->errorCode = errorCode;
    priv->errorSource = NoErrorSource;
    priv->errorData = QString();
}

QuillError::QuillError(ErrorCode errorCode,
                       ErrorSource errorSource,
                       const QString &errorData) :
    priv(new QuillErrorPrivate)
{
    priv->errorCode = errorCode;
    priv->errorSource = errorSource;
    priv->errorData = errorData;
}

QuillError::QuillError(const QuillError &quillError) :
    priv(new QuillErrorPrivate)
{
    priv->errorCode = quillError.errorCode();
    priv->errorSource = quillError.errorSource();
    priv->errorData = quillError.errorData();
}

QuillError::~QuillError()
{
    delete priv;
}

QuillError::ErrorCode QuillError::errorCode() const
{
    return priv->errorCode;
}

void QuillError::setErrorCode(QuillError::ErrorCode errorCode)
{
    priv->errorCode = errorCode;
}

QuillError::ErrorSource QuillError::errorSource() const
{
    return priv->errorSource;
}

void QuillError::setErrorSource(QuillError::ErrorSource errorSource)
{
    priv->errorSource = errorSource;
}

QString QuillError::errorData() const
{
    return priv->errorData;
}

void QuillError::setErrorData(QString errorData)
{
    priv->errorData = errorData;
}

QuillError QuillError::operator=(const QuillError &quillError)
{
    priv->errorCode = quillError.errorCode();
    priv->errorSource = quillError.errorSource();
    priv->errorData = quillError.errorData();
    return *this;
}

// Translates Quill image filter errors to Quill errors

QuillError::ErrorCode QuillError::translateFilterError(QuillImageFilter::ImageFilterError error)
{
    switch(error) {
    case QuillImageFilter::NoError:
        return QuillError::NoError;
    case QuillImageFilter::FileNotFoundError:
        return QuillError::FileNotFoundError;
    case QuillImageFilter::UnsupportedFormatError:
        return QuillError::FileFormatUnsupportedError;
    case QuillImageFilter::InvalidDataError:
        return QuillError::FileCorruptError;
    default:
        return QuillError::UnspecifiedError;
    }
}
