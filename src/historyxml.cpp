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

QByteArray HistoryXml::encode(File *file)
{
    QList<File *> files;
    files += file;
    return encode(files);
}

File *HistoryXml::decodeOne(const QByteArray & array)
{
    QList<File *> fileList = HistoryXml::decode(array);
    if (!fileList.isEmpty())
        return fileList.first();
    else
        return 0;
}


QByteArray HistoryXml::encode(QList<File *> files)
{
    QByteArray result;

    QXmlStreamWriter writer(&result);

    writer.writeStartDocument();
    writer.writeDTD("<!DOCTYPE QuillEditHistory>");

    writer.writeStartElement("", "Core");

    foreach (File *file, files)
    {
        QuillUndoStack *stack = file->stack();

        writer.writeStartElement("", "QuillUndoStack");

        writer.writeStartElement("", "File");
        writer.writeAttribute("name", file->fileName());
        writer.writeAttribute("format", file->targetFormat());
        writer.writeEndElement();

        writer.writeStartElement("", "OriginalFile");
        writer.writeAttribute("name", file->originalFileName());
        writer.writeAttribute("format", file->fileFormat());
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

        writer.writeTextElement("", "TargetIndex",
                                QString::number(targetIndex));

        writer.writeTextElement("", "SavedIndex",
                                QString::number(saveIndex));
        //for revert
        writer.writeTextElement("", "RevertIndex",
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
                    writer.writeStartElement("", "Session");
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
    writer->writeStartElement("", "Filter");

    writer->writeAttribute("name", filter->name());

    for (QuillImageFilter::QuillFilterOption option =
             QuillImageFilter::FirstQuillFilterOption;
         option <= QuillImageFilter::LastQuillFilterOption;
         option = (QuillImageFilter::QuillFilterOption)(option + 1))
    {
        if (filter->supportsOption(option))
        {
            const QVariant value = filter->option(option);

            writer->writeStartElement("", "FilterOption");
            writer->writeAttribute("name", QString::number(option));
            writer->writeAttribute("type", QString::number(value.type()));

            if (!writeComplexType(value, writer))
                writer->writeCharacters(value.toString());

            writer->writeEndElement();
        }
    }
    writer->writeEndElement();
}

bool HistoryXml::writeComplexType(const QVariant &variant, QXmlStreamWriter *writer)
{
    switch (variant.type())
    {
    case QVariant::Point:
    case QVariant::PointF:
        writer->writeStartElement("", "QPoint");
        writer->writeAttribute("", "x",
                               QString::number(variant.toPoint().x()));
        writer->writeAttribute("", "y",
                               QString::number(variant.toPoint().y()));
        writer->writeEndElement();
        break;

    case QVariant::Size:
        writer->writeStartElement("", "QSize");
        writer->writeAttribute("", "width",
                               QString::number(variant.toSize().width()));
        writer->writeAttribute("", "height",
                               QString::number(variant.toSize().height()));
        writer->writeEndElement();
        break;

    case QVariant::Rect:
        writer->writeStartElement("", "QRect");
        writer->writeAttribute("", "left",
                               QString::number(variant.toRect().left()));
        writer->writeAttribute("", "top",
                               QString::number(variant.toRect().top()));
        writer->writeAttribute("", "width",
                               QString::number(variant.toRect().width()));
        writer->writeAttribute("", "height",
                               QString::number(variant.toRect().height()));
        writer->writeEndElement();
        break;

    case QVariant::Color:
        writer->writeStartElement("", "QRgba");
        writer->writeAttribute("", "red",
                               QString::number(variant.value<QColor>().red()));
        writer->writeAttribute("", "green",
                               QString::number(variant.value<QColor>().green()));
        writer->writeAttribute("", "blue",
                               QString::number(variant.value<QColor>().blue()));
        writer->writeAttribute("", "alpha",
                               QString::number(variant.value<QColor>().alpha()));
        writer->writeEndElement();
        break;

    case QVariant::Polygon:
    {
        const QPolygon poly = variant.value<QPolygon>();
        writer->writeStartElement("", "QPolygon");
        for (int i=0; i<poly.count(); i++)
            writeComplexType(QVariant(poly.at(i)), writer);
        writer->writeEndElement();
        break;
    }

    // Not implemented yet.
    case QVariant::Font:
        writer->writeStartElement("", "QFont");
        writer->writeEndElement();
        break;


    // Not implemented yet.
    case QVariant::Pen:
        writer->writeStartElement("", "QPen");
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

QList<File*> HistoryXml::decode(const QByteArray & array)
{
    QXmlStreamReader reader(array);

    QList<File*> emptyList, files;
    QXmlStreamReader::TokenType token;

    if (readToken(&reader) != QXmlStreamReader::StartDocument)
        return emptyList;

    if (readToken(&reader) != QXmlStreamReader::DTD)
        return emptyList;

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != "Core"))
        return emptyList;

    token = readToken(&reader);
    while (token != QXmlStreamReader::EndElement)
    {
        if ((token != QXmlStreamReader::StartElement) ||
            (reader.name() != "QuillUndoStack"))
            return emptyList;

        if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
            (reader.name() != "File"))
            return emptyList;

        const QString fileName = reader.attributes().value("", "name").toString();
        const QString targetFormat = reader.attributes().value("", "format").toString();

        if (readToken(&reader) != QXmlStreamReader::EndElement)
            return emptyList;

        if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
            (reader.name() != "OriginalFile"))
            return emptyList;
        const QString originalFileName = reader.attributes().value("", "name").toString();
        const QString fileFormat = reader.attributes().value("", "format").toString();

        if (readToken(&reader) != QXmlStreamReader::EndElement)
            return emptyList;

        if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
            (reader.name() != "TargetIndex"))
            return emptyList;

        if (readToken(&reader) != QXmlStreamReader::Characters)
            return emptyList;
        int targetIndex = reader.text().toString().toInt();

        if (readToken(&reader) != QXmlStreamReader::EndElement)
            return emptyList;

        if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
            (reader.name() != "SavedIndex"))
            return emptyList;

        if (readToken(&reader) != QXmlStreamReader::Characters)
            return emptyList;
        int savedIndex = reader.text().toString().toInt();

        if (readToken(&reader) != QXmlStreamReader::EndElement)
            return emptyList;
        
        if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
            (reader.name() != "RevertIndex"))
            return emptyList;

        if (readToken(&reader) != QXmlStreamReader::Characters)
            return emptyList;
        int revertIndex = reader.text().toString().toInt();

        if (readToken(&reader) != QXmlStreamReader::EndElement)
            return emptyList;

        File *file = new File();
        file->setFileName(fileName);
        file->setOriginalFileName(originalFileName);
        file->setFileFormat(fileFormat);
        file->setTargetFormat(targetFormat);

        QuillUndoStack *stack = file->stack();

        stack->load();

        readEditSession(&reader, stack, &savedIndex, &targetIndex, fileName);

        // Undo the commands which were undone before the crash.
        while (stack->index()-1 > targetIndex)
            stack->undo();

        // If a load filter was inserted, treat indexes accordingly.
        if (savedIndex > 0)
            stack->setSavedIndex(savedIndex+1);
        else
            stack->setSavedIndex(0);
        stack->setRevertIndex(revertIndex);
        files.append(file);

        token = readToken(&reader);
    }

    if (readToken(&reader) != QXmlStreamReader::EndDocument)
        return emptyList;

    return files;
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
            ((reader->name() != "Filter") && (reader->name() != "Session")))
            return;

        if (reader->name() == "Filter") {
            QuillImageFilter *filter = readFilter(reader);

            if (filter != 0) {

                stack->add(filter);

                if (stack->index()-1 == *savedIndex) {

                    filter =
                        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Load);
                    filter->setOption(QuillImageFilter::FileName, fileName);
                    filter->setOption(QuillImageFilter::BackgroundColor,
                                      Core::instance()->backgroundRenderingColor());
                    stack->add(filter);
                    (*targetIndex)++;
                }
            }
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

    const QString name = reader->attributes().value("", "name").toString();

    QuillImageFilter *filter = QuillImageFilterFactory::createImageFilter(name);

    if (!filter)
        return 0;

    QXmlStreamReader::TokenType token = reader->readNext();
    while (token != QXmlStreamReader::EndElement)
    {
        if ((token != QXmlStreamReader::StartElement) ||
            (reader->name() != "FilterOption"))
        {
            success = false;
            break;
        }

        int option =
            reader->attributes().value("", "name").toString().toInt();
        int optionType =
            reader->attributes().value("", "type").toString().toInt();

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
        else
        {
            success = false;
            break;
        }
        filter->setOption((QuillImageFilter::QuillFilterOption) option, value);

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
        if (string == "false")
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
        if ((reader->name() != "QPoint"))
            return QVariant();

        int x =
            reader->attributes().value("", "x").toString().toInt();
        int y =
            reader->attributes().value("", "y").toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QPoint(x, y));
    }
    case QVariant::Size:
    {
        if (reader->name() != "QSize")
            return QVariant();

        int width =
            reader->attributes().value("", "width").toString().toInt();
        int height =
            reader->attributes().value("", "height").toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QSize(width, height));
    }
    case QVariant::Rect:
    {
        if (reader->name() != "QRect")
            return QVariant();

        int left =
            reader->attributes().value("", "left").toString().toInt();
        int top =
            reader->attributes().value("", "top").toString().toInt();
        int width =
            reader->attributes().value("", "width").toString().toInt();
        int height =
            reader->attributes().value("", "height").toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QRect(left, top, width, height));
    }
    case QVariant::Color:
    {
        if (reader->name() != "QRgba")
            return QVariant();

        int red =
            reader->attributes().value("", "red").toString().toInt();
        int green =
            reader->attributes().value("", "green").toString().toInt();
        int blue =
            reader->attributes().value("", "blue").toString().toInt();
        int alpha =
            reader->attributes().value("", "alpha").toString().toInt();

        if (reader->readNext() != QXmlStreamReader::EndElement)
            return QVariant();

        return QVariant(QColor(red, green, blue, alpha));
    }

    case QVariant::Polygon:
    {
        if (reader->name() != "QPolygon")
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

