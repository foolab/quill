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

#ifndef STRINGS_H
#define STRINGS_H

namespace Strings
{
#define S(var, val) static const QLatin1String var (val)

    // Please keep sorted by variable name

    S(aviMimeType,           "video/avi");

    S(dot,                   ".");
    S(dotXml,                ".xml");

    S(gifMimeType,           "image/gif");

    S(historyPath,           "/.config/quill/history");

    S(jpegMimeType,          "image/jpeg");
    S(jpeg,                  "jpeg");
    S(jpg,                   "jpg");

    S(mp4MimeType,           "video/mp4");

    S(pngMimeType,           "image/png");
    S(png,                   "png");

    S(slash,                 "/");
    S(slashOriginal,         "/.original/");
    S(svgMimeType,           "image/svg+xml");

    S(tempDirDefault,        "/tmp");
    S(tempFilePattern,       "qt_temp.XXXXXX.");
    S(thumbsBasePath,        "/.thumbnails");
    S(thumbsFail,            "/fail/quill");
    S(thumbsNormal,          "/.thumbnails/normal");
    S(thumbsScreen,          "/.thumbnails/screen");
    S(thumbsWide,            "/.thumbnails/wide");
    S(testsTempDir,          "/.config/quill/tmp/");
    S(testsTempFilePattern,  "/.config/quill/tmp/XXXXXX");

    S(xmlAttributeAlpha,     "alpha");
    S(xmlAttributeBlue,      "blue");
    S(xmlAttributeFormat,    "format");
    S(xmlAttributeGreen,     "green");
    S(xmlAttributeHeight,    "height");
    S(xmlAttributeLeft,      "left");
    S(xmlAttributeName,      "name");
    S(xmlAttributeRed,       "red");
    S(xmlAttributeTop,       "top");
    S(xmlAttributeType,      "type");
    S(xmlAttributeWidth,     "width");
    S(xmlAttributeX,         "x");
    S(xmlAttributeY,         "y");
    S(xmlHistoryDoctype,     "<!DOCTYPE QuillEditHistory>");
    S(xmlNamespace,          "");
    S(xmlNodeCore,           "Core");
    S(xmlNodeFile,           "File");
    S(xmlNodeFilter,         "Filter");
    S(xmlNodeFilterOption,   "FilterOption");
    S(xmlNodeOriginalFile,   "OriginalFile");
    S(xmlNodeQFont,          "QFont");
    S(xmlNodeQPen,           "QPen");
    S(xmlNodeQPoint,         "QPoint");
    S(xmlNodeQPolygon,       "QPolygon");
    S(xmlNodeQRect,          "QRect");
    S(xmlNodeQRgba,          "QRgba");
    S(xmlNodeQSize,          "QSize");
    S(xmlNodeQuillUndoStack, "QuillUndoStack");
    S(xmlNodeRevertIndex,    "RevertIndex");
    S(xmlNodeSavedIndex,     "SavedIndex");
    S(xmlNodeSession,        "Session");
    S(xmlNodeTargetIndex,    "TargetIndex");
    S(xmlValueFalse,         "false");

#undef S
};

#endif /* !STRINGS_H */

