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

/*!
  \class HistoryXml

  \brief Responsible for conversion between QuillFile objects and XML.

Most of the effort comes from edit history conversion. HistoryXml is
used when recovering edit history of a file with modifications to
allow undo and redo even between different application runs. In
addition, HistoryXml can be used in crash recovery situations to
recover all edits up to the point of crash.

HistoryXML does not (yet?) support Qt 4.6's XML schema checking capabilities.
 */

#ifndef HISTORYXML_H
#define HISTORYXML_H

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QVariant>

class File;
class QuillImageFilter;
class QuillUndoStack;
class Core;

class HistoryXml
{
    friend class ut_xml;

public:
    static QByteArray encode(File *file);
    static QByteArray encode(QList<File *> files);
    static File *decodeOne(const QByteArray & array);
    static QList<File *> decode(const QByteArray & array);
    static bool decodeOne(const QByteArray & array,File* file);
    static bool decode(const QByteArray & array,File* file);

private:
    static void writeFilter(QuillImageFilter *filter, QXmlStreamWriter *writer);
    static bool writeComplexType(const QVariant &variant, QXmlStreamWriter *writer);
    static QXmlStreamReader::TokenType readToken(QXmlStreamReader *reader);
    static void readEditSession(QXmlStreamReader *reader, QuillUndoStack *stack,
                                int *savedIndex, int *targetIndex,
                                const QString &fileName);
    static QuillImageFilter *readFilter(QXmlStreamReader *reader);
    static QVariant recoverVariant(QVariant::Type variantType, const QString &string);
    static QVariant recoverComplexType(QVariant::Type variantType, QXmlStreamReader *reader);
    static bool decodeEditHistory(QXmlStreamReader& reader,QXmlStreamReader::TokenType& token,
                                  int& saveIndex,int& targetIndex,int& revertIndex,QString& fileName,
                                  QString& originalFileName, QString& fileFormat,
                                  QString& targetFormat);
    static bool readEditHistoryHeader(QXmlStreamReader& reader,QXmlStreamReader::TokenType& token);
    static void handleStack(QXmlStreamReader& reader, QuillUndoStack *stack,
                            int& savedIndex, int& targetIndex,
                            const QString& fileName, int&revertIndex);
};


#endif
