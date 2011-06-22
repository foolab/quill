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
    std::cout << "Generating " << n <<" "<< flavor.toAscii().constData() << size.width() << "x" << size.height() << " thumbnails for " << originalFileName.toAscii().constData() << " MIME " << mimeType.toAscii().constData() << "\n";

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
