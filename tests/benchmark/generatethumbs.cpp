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

#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QTime>
#include <QTemporaryFile>

#include <Quill>
#include <QuillFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <iostream>

#include "../../src/strings.h"

void generateThumbs(QString originalFileName, int n, QSize size, QSize minimumSize, QString mimeType, QString flavor)
{
    std::cout << "Generating " << n <<" "<< flavor.toAscii().constData() << size.width() << "x" << size.height() << " thumbnails for " << originalFileName.toLocal8Bit().constData() << " MIME " << mimeType.toAscii().constData() << "\n";

    QEventLoop loop;
    QTime time;

    Quill::setTemporaryFilePath(QDir::homePath() + Strings::testsTempDir);
    Quill::setPreviewSize(0, size);
    Quill::setMinimumPreviewSize(0, minimumSize);
    Quill::setThumbnailFlavorName(0, flavor);
    Quill::setThumbnailExtension(Strings::jpeg);
    Quill::setThumbnailCreationEnabled(false);

    int numFiles = n;

    QString fileName[numFiles];
    QuillFile *quillFile[numFiles];

    for (int i=0; i<numFiles; i++) {
        {   // Needed for the life of the QTemporaryFile
            QTemporaryFile file;
            file.setFileTemplate(QDir::homePath() + Strings::testsTempFilePattern);
            file.open();
            fileName[i] = file.fileName();
            file.close();
        }
        QFile::remove(fileName[i]);
        QFile::copy(originalFileName, fileName[i]);
    }

    time.start();

    for (int i=0; i<numFiles; i++) {
        quillFile[i] = new QuillFile(fileName[i], mimeType);
        QObject::connect(quillFile[i], SIGNAL(imageAvailable(const QuillImageList)),
                         &loop, SLOT(quit()));
    }

    int initTime = time.elapsed();

    for (int i=0; i<numFiles; i++)
        quillFile[i]->setDisplayLevel(0);

    int displayLevelTime = time.elapsed();

    do
        loop.exec();
    while (Quill::isCalculationInProgress());

    int finalTime = time.elapsed();

    for (int i=0; i<numFiles; i++)
        if (quillFile[i]->image(0).isNull()) {
            std::cout<<"Error: not all images are loaded!\n";
            return;
        }

    std::cout << "Initialize " << numFiles << " QuillFiles: "
             << initTime << "ms" << "\n";

    std::cout << "Set display levels of " << numFiles << " QuillFiles: "
             << displayLevelTime - initTime << "ms" << "\n";

    std::cout << "Use case generate " << numFiles << " thumbnails: "
             << finalTime << "ms" << "\n";

    for (int i=0; i<numFiles; i++) {
        delete quillFile[i];
        QFile::remove(fileName[i]);
    }
}
