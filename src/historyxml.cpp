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

#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QFile>
#include <QColor>
#include <QPen>
#include <QFont>
#include <QPolygon>

#include "core.h"
#include "historyxml.h"
#include "file.h"
#include "quillundostack.h"
#include "quillundocommand.h"
#include "strings.h"

QByteArray HistoryXml::encode(File *file)
{
    QList<File *> files;
    files += file;
    return encode(files);
}

bool HistoryXml::decodeOne(const QByteArray & array,File* file)
{
    return  HistoryXml::decode(array,file);
}


QByteArray HistoryXml::encode(QList<File *> files)
{
    QByteArray result;

    QXmlStreamWriter writer(&result);

    writer.writeStartDocument();
    writer.writeDTD(Strings::xmlHistoryDoctype);

    writer.writeStartElement(Strings::xmlNamespace,
                             Strings::xmlNodeCore);

    foreach (File *file, files)
    {
        QuillUndoStack *stack = file->stack();

        writer.writeStartElement(Strings::xmlNamespace,
                                 Strings::xmlNodeQuillUndoStack);

        writer.writeStartElement(Strings::xmlNamespace,
                                 Strings::xmlNodeFile);
        writer.writeAttribute(Strings::xmlAttributeName, file->fileName());
        writer.writeAttribute(Strings::xmlAttributeFormat, file->targetFormat());
        writer.writeEndElement();

        writer.writeStartElement(Strings::xmlNamespace,
                                 Strings::xmlNodeOriginalFile);
        writer.writeAttribute(Strings::xmlAttributeName, file->originalFileName());
        writer.writeAttribute(Strings::xmlAttributeFormat, file->fileFormat());
        writer.writeEndElement();

        int targetIndex = 0;
        if (stack->command())
            targetIndex = stack->command()->index();
        int saveIndex = stack->savedIndex();
        //for revert
        int revertIndex = stack->revertIndex();
        //end
        // Load filters are not saved and do not affect indexes in the dump.

        for (int i = 1; i < stack->index(); i++)
            if (!stack->command(i)->filter() ||
                stack->command(i)->filter()->role() == QuillImageFilter::Role_Load)
            {
                targetIndex--;
                saveIndex--;
            }

        writer.writeTextElement(Strings::xmlNamespace,
                                Strings::xmlNodeTargetIndex,
                                QString::number(targetIndex));

        writer.writeTextElement(Strings::xmlNamespace,
                                Strings::xmlNodeSavedIndex,
                                QString::number(saveIndex));
        //for revert
        writer.writeTextElement(Strings::xmlNamespace,
                                Strings::xmlNodeRevertIndex,
                                QString::number(revertIndex));
        //end
        bool isSession = false;;
        int sessionId = 0;

        for (int i = 0; i < stack->count(); i++)
            if (stack->command(i)->filter() &&
                stack->command(i)->filter()->role() != QuillImageFilter::Role_Load)
            {
                if (isSession &&
                    !stack->command(i)->belongsToSession(sessionId)) {
                    writer.writeEndElement();
                    isSession = false;
                }
                if (!isSession && stack->command(i)->belongsToSession()) {
                    writer.writeStartElement(Strings::xmlNamespace,
                                             Strings::xmlNodeSession);
                    isSession = true;
                    sessionId = stack->command(i)->sessionId();
                }

                writeFilter(stack->command(i)->filter(), &writer);
            }

        if (isSession)
            writer.writeEndElement();

        writer.writeEndElement();
    }

    writer.writeEndElement();

        writer.writeEndDocument();

        return result;
    }

void HistoryXml::writeFilter(QuillImageFilter *filter, QXmlStreamWriter *writer)
{
    writer->writeStartElement(Strings::xmlNamespace,
                              Strings::xmlNodeFilter);

    writer->writeAttribute(Strings::xmlAttributeName, filter->name());

    foreach (QString option, filter->supportedOptions()) {
        const QVariant value = filter->option(option);

        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeFilterOption);
        writer->writeAttribute(Strings::xmlAttributeName, option);
        writer->writeAttribute(Strings::xmlAttributeType, QString::number(value.type()));

        if (!writeComplexType(value, writer))
            writer->writeCharacters(value.toString());

        writer->writeEndElement();
    }
    writer->writeEndElement();
}

bool HistoryXml::writeComplexType(const QVariant &variant, QXmlStreamWriter *writer)
{
    switch (variant.type())
    {
    case QVariant::Point:
    case QVariant::PointF:
        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeQPoint);
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeX,
                               QString::number(variant.toPoint().x()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeY,
                               QString::number(variant.toPoint().y()));
        writer->writeEndElement();
        break;

    case QVariant::Size:
        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeQSize);
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeWidth,
                               QString::number(variant.toSize().width()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeHeight,
                               QString::number(variant.toSize().height()));
        writer->writeEndElement();
        break;

    case QVariant::Rect:
        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeQRect);
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeLeft,
                               QString::number(variant.toRect().left()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeTop,
                               QString::number(variant.toRect().top()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeWidth,
                               QString::number(variant.toRect().width()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeHeight,
                               QString::number(variant.toRect().height()));
        writer->writeEndElement();
        break;

    case QVariant::Color:
        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeQRgba);
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeRed,
                               QString::number(variant.value<QColor>().red()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeGreen,
                               QString::number(variant.value<QColor>().green()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeBlue,
                               QString::number(variant.value<QColor>().blue()));
        writer->writeAttribute(Strings::xmlNamespace,
                               Strings::xmlAttributeAlpha,
                               QString::number(variant.value<QColor>().alpha()));
        writer->writeEndElement();
        break;

    case QVariant::Polygon:
    {
        const QPolygon poly = variant.value<QPolygon>();
        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeQPolygon);
        for (int i=0; i<poly.count(); i++)
            writeComplexType(QVariant(poly.at(i)), writer);
        writer->writeEndElement();
        break;
    }

    // Not implemented yet.
    case QVariant::Font:
        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeQFont);
        writer->writeEndElement();
        break;


    // Not implemented yet.
    case QVariant::Pen:
        writer->writeStartElement(Strings::xmlNamespace,
                                  Strings::xmlNodeQPen);
        writer->writeEndElement();
        break;

    default:
        return false;
    }
    return true;
}

QXmlStreamReader::TokenType HistoryXml::readToken(QXmlStreamReader *reader)
{
    QXmlStreamReader::TokenType tokenType;
    do
        tokenType = reader->readNext();
    while (reader->isWhitespace());
    return tokenType;
}

bool HistoryXml::decode(const QByteArray & array,File* file)
{
    QXmlStreamReader reader(array);
    QXmlStreamReader::TokenType token;

    int savedIndex = 0;
    int targetIndex = 0;
    int revertIndex = 0;
    QString fileName = QString();
    QString originalFileName = QString();
    QString fileFormat = QString();
    QString targetFormat = QString();

    if(!readEditHistoryHeader(reader,token))
        return false;

    // If stack was originally setup with just the load filter, already
    // loaded images need to be kept
    bool hadCommand = false;
    int commandId = 0;
    QSize fullImageSize;
    QList<QuillImage> imageList;

    if (!file->stack()->isClean()) {
        hadCommand = true;
        commandId = file->stack()->command()->uniqueId();
        fullImageSize = file->stack()->command()->fullImageSize();
        for (int i=0; i<=Core::instance()->previewLevelCount(); i++)
            imageList << file->stack()->command()->image(i);
    }

    // This method will overwrite the stack, let's make sure that we
    // start with a clean one

    file->stack()->clear();

    while (token != QXmlStreamReader::EndElement)
    {
        if(!decodeEditHistory(reader,token,savedIndex,targetIndex,
                              revertIndex,fileName,originalFileName,
                              fileFormat,targetFormat))
            return false;
        QuillUndoStack *stack = file->stack();

        stack->load();
        handleStack(reader, stack, savedIndex, targetIndex, fileName,revertIndex);
        // Redirecting images and ongoing operations from the old load
        // filter of the stack to the complete one
        if (hadCommand) {
            stack->command()->setUniqueId(commandId);
            stack->command()->setFullImageSize(fullImageSize);
            for (int i=0; i<=Core::instance()->previewLevelCount(); i++)
                stack->command()->setImage(i, imageList[i]);
        }

        token = readToken(&reader);
    }

    if (readToken(&reader) != QXmlStreamReader::EndDocument)
        return false;

    return true;
}

void HistoryXml::readEditSession(QXmlStreamReader *reader,
                                 QuillUndoStack *stack,
                                 int *savedIndex,
                                 int *targetIndex,
                                 const QString &fileName)
{
    QXmlStreamReader::TokenType token = readToken(reader);
    while (token != QXmlStreamReader::EndElement)
    {
        if ((token != QXmlStreamReader::StartElement) ||
            ((reader->name() != Strings::xmlNodeFilter) &&
             (reader->name() != Strings::xmlNodeSession)))
            return;

        if (reader->name() == Strings::xmlNodeFilter) {
            QuillImageFilter *filter = readFilter(reader);

            if (filter != 0)
                stack->add(filter);
        } else {
            stack->startSession();
            readEditSession(reader, stack, savedIndex, targetIndex, fileName);
            stack->endSession();
        }

        token = readToken(reader);
    }
}

QuillImageFilter *HistoryXml::readFilter(QXmlStreamReader *reader)
{
    bool success = true;

    const QString name = reader->attributes().value(Strings::xmlNamespace,
                                                    Strings::xmlAttributeName).toString();

    QuillImageFilter *filter = QuillImageFilterFactory::createImageFilter(name);

    if (!filter)
        return 0;

    QXmlStreamReader::TokenType token = reader->readNext();
    while (token != QXmlStreamReader::EndElement)
    {
        if ((token != QXmlStreamReader::StartElement) ||
            (reader->name() != Strings::xmlNodeFilterOption))
        {
            success = false;
            break;
        }

        QString option = reader->attributes().value(Strings::xmlNamespace,
                                                    Strings::xmlAttributeName).toString();
        int optionType = reader->attributes().value(Strings::xmlNamespace,
                                                    Strings::xmlAttributeType).toString().toInt();

        token = reader->readNext();

        QVariant value;

        if (token == QXmlStreamReader::Characters)
        {
            QString string = reader->text().toString();

            value = recoverVariant((QVariant::Type)optionType, string);

            if (!value.isValid())
            {
                success = false;
                break;
            }

        }
        else if (token == QXmlStreamReader::StartElement)
        {
            value = recoverComplexType((QVariant::Type)optionType, reader);

            if (!value.isValid())
            {
                success = false;
                break;
            }
        }
        // FilterOption with empty content (like in eye reduction)
        else if (token == QXmlStreamReader::EndElement &&
                 reader->name() == Strings::xmlNodeFilterOption)
        {
            filter->setOption(option, value);
            token = reader->readNext();
            continue;

        } else {
            success = false;
            break;
        }
        filter->setOption(option, value);

        if (reader->readNext() != QXmlStreamReader::EndElement)
        {
            success = false;
            break;
        }

        token = reader->readNext();
    }

    if (!success)
    {
        delete filter;
        return 0;
    }

    return filter;
}

QVariant HistoryXml::recoverVariant(QVariant::Type variantType, const QString &string)
{
    bool ok;
    int toInt = string.toInt(&ok);
    if (ok)
        return toInt;
    else {
        float toFloat = string.toFloat(&ok);
        if (ok)
            return toFloat;
    }

    switch (variantType) {
    case QVariant::String:
        return QVariant(string);
    case QVariant::Bool:
        if (string == Strings::xmlValueFalse)
            return QVariant(false);
        else
            return QVariant(true);
    default:
        return QVariant();
    }
}

QVariant HistoryXml::recoverComplexType(QVariant::Type variantType,
                                        QXmlStreamReader *reader)
{
    switch (variantType)
    {
    case QVariant::Point:
    case QVariant::PointF:
    {
        if ((reader->name() != Strings::xmlNodeQPoint))
            return QVariant();

        int x = reader->attributes().value(Strings::xmlNamespace,
                                           Strings::xmlAttributeX).toString().toInt();
        int y = reader->attributes().value(Strings::xmlNamespace,
                                           Strings::xmlAttributeY).toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QPoint(x, y));
    }
    case QVariant::Size:
    {
        if (reader->name() != Strings::xmlNodeQSize)
            return QVariant();

        int width  = reader->attributes().value(Strings::xmlNamespace,
                                                Strings::xmlAttributeWidth).toString().toInt();
        int height = reader->attributes().value(Strings::xmlNamespace,
                                                Strings::xmlAttributeHeight).toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QSize(width, height));
    }
    case QVariant::Rect:
    {
        if (reader->name() != Strings::xmlNodeQRect)
            return QVariant();

        int left   = reader->attributes().value(Strings::xmlNamespace,
                                                Strings::xmlAttributeLeft).toString().toInt();
        int top    = reader->attributes().value(Strings::xmlNamespace,
                                                Strings::xmlAttributeTop).toString().toInt();
        int width  = reader->attributes().value(Strings::xmlNamespace,
                                                Strings::xmlAttributeWidth).toString().toInt();
        int height = reader->attributes().value(Strings::xmlNamespace,
                                                Strings::xmlAttributeHeight).toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QRect(left, top, width, height));
    }
    case QVariant::Color:
    {
        if (reader->name() != Strings::xmlNodeQRgba)
            return QVariant();

        int red   = reader->attributes().value(Strings::xmlNamespace,
                                               Strings::xmlAttributeRed).toString().toInt();
        int green = reader->attributes().value(Strings::xmlNamespace,
                                               Strings::xmlAttributeGreen).toString().toInt();
        int blue  = reader->attributes().value(Strings::xmlNamespace,
                                               Strings::xmlAttributeBlue).toString().toInt();
        int alpha = reader->attributes().value(Strings::xmlNamespace,
                                               Strings::xmlAttributeAlpha).toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QColor(red, green, blue, alpha));
    }

    case QVariant::Polygon:
    {
        if (reader->name() != Strings::xmlNodeQPolygon)
            return QVariant();

        QPolygon poly;

        while (reader->readNext() != QXmlStreamReader::EndElement)
        {
            QPoint point = recoverComplexType(QVariant::Point, reader).
                value<QPoint>();

            if (point == QPoint()) {
                poly = QPolygon();
                break;
            }
            poly.append(point);
        }

        return QVariant(poly);
    }

    // Not implemented yet
    case QVariant::Font:
    {
        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QFont());
    }

    // Not implemented yet
    case QVariant::Pen:
    {
        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QPen());
    }

    default:
        return QVariant();
    }
}

QList<File*> HistoryXml::decode(const QByteArray & array)
{
    QXmlStreamReader reader(array);
    QList<File*> emptyList, files;
    QXmlStreamReader::TokenType token;

    int savedIndex = 0;
    int targetIndex = 0;
    int revertIndex = 0;
    QString fileName = QString();
    QString originalFileName = QString();
    QString fileFormat = QString();
    QString targetFormat = QString();

    if(!readEditHistoryHeader(reader,token))
        return emptyList;

    while (token != QXmlStreamReader::EndElement)
    {
        if(!decodeEditHistory(reader,token,savedIndex,targetIndex,revertIndex,
                              fileName,originalFileName,fileFormat,targetFormat))
            return emptyList;

        File *file = new File();
        file->setFileName(fileName);
        file->setOriginalFileName(originalFileName);
        file->setFileFormat(fileFormat);
        file->setTargetFormat(targetFormat);

        QuillUndoStack *stack = file->stack();

        stack->load();

        handleStack(reader, stack, savedIndex, targetIndex, fileName,revertIndex);

        files.append(file);

        token = readToken(&reader);

    }

    if (readToken(&reader) != QXmlStreamReader::EndDocument)
        return emptyList;

    return files;
}

File *HistoryXml::decodeOne(const QByteArray & array)
{
     QList<File *> fileList = HistoryXml::decode(array);
    if (!fileList.isEmpty())
        return fileList.first();
    else
        return 0;
}

bool HistoryXml::decodeEditHistory(QXmlStreamReader& reader,QXmlStreamReader::TokenType& token,
                                   int& savedIndex,int& targetIndex,int& revertIndex,QString& fileName,
                                   QString& originalFileName, QString& fileFormat,
                                   QString& targetFormat)
{
    if ((token != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeQuillUndoStack))
        return false;

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeFile))
        return false;

    fileName     = reader.attributes().value(Strings::xmlNamespace,
                                             Strings::xmlAttributeName).toString();
    targetFormat = reader.attributes().value(Strings::xmlNamespace,
                                             Strings::xmlAttributeFormat).toString();

    if (readToken(&reader) != QXmlStreamReader::EndElement)
        return false;

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeOriginalFile))
        return false;

    originalFileName = reader.attributes().value(Strings::xmlNamespace,
                                                 Strings::xmlAttributeName).toString();
    fileFormat       = reader.attributes().value(Strings::xmlNamespace,
                                                 Strings::xmlAttributeFormat).toString();

    if (readToken(&reader) != QXmlStreamReader::EndElement)
        return false;

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeTargetIndex))
        return false;

    if (readToken(&reader) != QXmlStreamReader::Characters)
        return false;
    targetIndex = reader.text().toString().toInt();

    if (readToken(&reader) != QXmlStreamReader::EndElement)
        return false;

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeSavedIndex))
        return false;

    if (readToken(&reader) != QXmlStreamReader::Characters)
        return false;
    savedIndex = reader.text().toString().toInt();

    if (readToken(&reader) != QXmlStreamReader::EndElement)
        return false;

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeRevertIndex))
        return false;

    if (readToken(&reader) != QXmlStreamReader::Characters)
        return false;
    revertIndex = reader.text().toString().toInt();

    if (readToken(&reader) != QXmlStreamReader::EndElement)
        return false;

    return true;
}


bool HistoryXml::readEditHistoryHeader(QXmlStreamReader& reader,QXmlStreamReader::TokenType& token)
{
    if (readToken(&reader) != QXmlStreamReader::StartDocument)
        return false;

    if (readToken(&reader) != QXmlStreamReader::DTD)
        return false;

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeCore))
        return false;

    token = readToken(&reader);
    return true;
}

void HistoryXml::handleStack(QXmlStreamReader& reader, QuillUndoStack *stack,
                            int& savedIndex, int& targetIndex,
                            const QString& fileName, int&revertIndex)
{
    readEditSession(&reader, stack, &savedIndex, &targetIndex, fileName);

    // Undo the commands which were undone before the crash.
    while (stack->index()-1 > targetIndex)
        stack->undo();

    stack->setSavedIndex(savedIndex);
    stack->setRevertIndex(revertIndex);

}
