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

#include <QtGlobal>
#include <QUrl>
#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QDebug>

#include "autoclean.h"
#include "../src/logger.h"
#include "../src/strings.h"

Autoclean::Autoclean() :
    m_editHistoryPath(QDir::homePath() + Strings::historyPath)
{
    m_thumbnailPaths.append(QDir::homePath() + Strings::thumbsNormal);
    qsrand(QDateTime::currentDateTime().toTime_t());
}

void Autoclean::setEditHistoryPath(const QString &editHistoryPath)
{
    m_editHistoryPath = editHistoryPath;
}

QString Autoclean::editHistoryPath() const
{
    return m_editHistoryPath;
}

void Autoclean::Autoclean::setThumbnailPaths(const QList<QString> &pathList)
{
    m_thumbnailPaths = pathList;
}

QList<QString> Autoclean::thumbnailPaths() const
{
    return m_thumbnailPaths;
}

void Autoclean::run(int nItemsToCheck) const
{
    const QStringList fileList = QDir(m_editHistoryPath).entryList();
    int nItems = fileList.count();

    if (nItemsToCheck > nItems)
        nItemsToCheck = nItems;

    QUILL_LOG("[Autoclean]", "Found" + QString::number(nItems) +
                " edit history files in " + m_editHistoryPath);

    if (nItems == 0)
        return;

    QUILL_LOG("[Autoclean]", "Scanning " + QString::number(nItems) +
                " edit history files.");

    int item = (qrand() / 10000) % nItems;

    for (;nItemsToCheck>0;nItemsToCheck--) {
        item++;
        if (item >= nItems)
            item = 0;

        const QString editHistoryFileName =
            m_editHistoryPath+"/"+fileList[item];
        const QString mainFileName = getMainFileName(editHistoryFileName);

        if (!QFile::exists(mainFileName) &&
            verifyMainFileName(mainFileName, editHistoryFileName)) {

            QUILL_LOG("[Autoclean]", "File " + mainFileName +
                        "no longer exists, deleting support files.");

            QFile::remove(originalFileName(mainFileName));
            removeThumbnails(mainFileName, m_thumbnailPaths);
            QFile::remove(editHistoryFileName);
        }
    }
}

QString Autoclean::getMainFileName(const QString &editHistoryFileName)
{
    QFile file(editHistoryFileName);
    file.open(QIODevice::ReadOnly);
    QByteArray buffer = file.readAll();

    QXmlStreamReader reader(buffer);

    if (readToken(&reader) != QXmlStreamReader::StartDocument)
        return QString();

    if (readToken(&reader) != QXmlStreamReader::DTD)
        return QString();

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeCore))
        return QString();

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeQuillUndoStack))
        return QString();

    if ((readToken(&reader) != QXmlStreamReader::StartElement) ||
        (reader.name() != Strings::xmlNodeFile))
        return QString();

    return reader.attributes().value(Strings::xmlNamespace,
                                     Strings::xmlAttributeName).toString();
}

bool Autoclean::verifyMainFileName(const QString &mainFileName,
                                 const QString &editHistoryFileName)
{
    return editHistoryFileName.contains(fileNameHash(mainFileName));
}

QString Autoclean::originalFileName(const QString &mainFileName)
{
    int split = mainFileName.lastIndexOf("/");
    return mainFileName.left(split) + Strings::slashOriginal + mainFileName.mid(split);
}

void Autoclean::removeThumbnails(const QString &mainFileName,
                               const QList<QString> &thumbnailPaths)
{
    foreach (QString thumbnailPath, thumbnailPaths)
        QFile::remove(thumbnailPath+"/"+fileNameHash(mainFileName)+".jpeg");
}

QString Autoclean::fileNameHash(const QString &fileName)
{
    const QUrl uri = QUrl::fromLocalFile(fileName);

    const QByteArray hashValue =
        QCryptographicHash::hash(uri.toString().toLatin1(),
                                 QCryptographicHash::Md5);

    return hashValue.toHex();
}

QXmlStreamReader::TokenType Autoclean::readToken(QXmlStreamReader *reader)
{
    QXmlStreamReader::TokenType tokenType;
    do
        tokenType = reader->readNext();
    while (reader->isWhitespace());
    return tokenType;
}
