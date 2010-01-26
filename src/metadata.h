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

#ifndef QUILL_METADATA_H
#define QUILL_METADATA_H

#include <libexif/exif-data.h>
#include <QString>

class Metadata
{
    friend class ut_metadata;

    enum Tag {
        Tag_Make = EXIF_TAG_MAKE,
        Tag_Model = EXIF_TAG_MODEL,
        Tag_ImageWidth = EXIF_TAG_IMAGE_WIDTH,
        Tag_ImageHeight = EXIF_TAG_IMAGE_LENGTH,
        Tag_FocalLength = EXIF_TAG_FOCAL_LENGTH,
        Tag_ExposureTime = EXIF_TAG_EXPOSURE_TIME,
        Tag_TimestampOriginal = EXIF_TAG_DATE_TIME_ORIGINAL
    };

 public:
    Metadata(const QString &fileName);
    ~Metadata();

    /*!
      Returns true if the metadata in the file was valid.
     */
    bool isValid();

    /*!
      Returns the value of the metadata entry for a given tag.
      Currently, only two tags are supported for testing purposes.
     */
    QVariant entry(Tag tag);

 private:
    ExifData *m_exifData;
    ExifByteOrder m_exifByteOrder;
};

#endif
