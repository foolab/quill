/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Alexander Bokovoy <alexander.bokovoy@nokia.com>
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

#include <QVariant>
#include <QDebug>

#include "metadata.h"

Metadata::Metadata(const QString &fileName)
{
    m_exifData = exif_data_new_from_file(fileName.toAscii().constData());
    m_exifByteOrder = exif_data_get_byte_order(m_exifData);
}

Metadata::~Metadata()
{
    exif_data_unref(m_exifData);
}

bool Metadata::isValid()
{
    return m_exifData != 0;
}

QVariant Metadata::entry(Metadata::Tag tag)
{
    if (!m_exifData)
        return QVariant();

    ExifEntry *entry = exif_data_get_entry(m_exifData, (ExifTag)tag);
    if (!entry)
        return QVariant();

    QVariant result;

    switch(entry->format) {
    case EXIF_FORMAT_ASCII:
        result = QVariant(QString((const char*)entry->data));
        break;

    case EXIF_FORMAT_SHORT:
        result = QVariant(exif_get_short(entry->data, m_exifByteOrder));
        break;

    case EXIF_FORMAT_LONG:
        result = QVariant(exif_get_long(entry->data, m_exifByteOrder));
        break;

    case EXIF_FORMAT_RATIONAL: {
        ExifRational rational = exif_get_rational(entry->data, m_exifByteOrder);
        if (rational.denominator == 0)
            result = QVariant();
        else
            result = (float)rational.numerator / (float)rational.denominator;
        break;
    }

    case EXIF_FORMAT_SRATIONAL: {
        ExifSRational srational = exif_get_srational(entry->data, m_exifByteOrder);
        if (srational.denominator == 0)
            result = QVariant();
        else
            result = (float)srational.numerator / (float)srational.denominator;
        break;
    }

    case EXIF_FORMAT_FLOAT:
        result = QVariant(*((float*)entry->data));
        break;

    case EXIF_FORMAT_DOUBLE:
        result = QVariant(*((float*)entry->data));
        break;

    default:
        result = QVariant();
    }

    return result;
}
