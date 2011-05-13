#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QTime>
#include <QDebug>
#include <QTemporaryFile>

#include <Quill>
#include <QuillFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "../../src/file.h"
#include "../../src/strings.h"

void loadThumbs(QString originalFileName, int n, QSize size)
{
    qDebug() << "Loading" << n << size.width() << "x" << size.height() << "generated thumbnails for" << originalFileName;

    QEventLoop loop;
    QTime time;

    Quill::setTemporaryFilePath(QDir::homePath() + Strings::testsTempDir);

    Quill::setThumbnailFlavorName(0, "quill-benchmark");

    Quill::setThumbnailExtension(Strings::jpg);
    Quill::setPreviewSize(0, size);

    int numFiles = n;

    QString fileName[numFiles];
    QuillFile *quillFile[numFiles];

    {
        QImage image = QImage(originalFileName).scaled(size);
        QTemporaryFile thumbFile;
        thumbFile.open();
        image.save(thumbFile.fileName(), "jpeg");

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

            QString thumbFileName = QDir::homePath() +
                "/.thumbnails/quill-benchmark/" +
                File::filePathHash(fileName[i]) + ".jpg";

            QFile::copy(thumbFile.fileName(), thumbFileName);
        }
    }

    time.start();

    for (int i=0; i<numFiles; i++) {
        quillFile[i] = new QuillFile(fileName[i], Strings::jpegMimeType);
        QObject::connect(quillFile[i], SIGNAL(imageAvailable(const QuillImageList)),
                         &loop, SLOT(quit()));
    }

    int initTime = time.elapsed();

    for (int i=0; i<numFiles; i++)
        quillFile[i]->setDisplayLevel(0);

    int displayLevelTime = time.elapsed();

    while (Quill::isCalculationInProgress())
        loop.exec();

    int finalTime = time.elapsed();

    for (int i=0; i<numFiles; i++)
        if (quillFile[i]->image(0).isNull()) {
            qDebug("Error: not all images are loaded!");
            return;
        }

    qDebug() << "Initialize" << numFiles << "QuillFiles:"
             << initTime << "ms";

    qDebug() << "Set display levels of" << numFiles << "QuillFiles:"
             << displayLevelTime - initTime << "ms";

    qDebug() << "Use case load" << numFiles << "generated thumbnails:"
             << finalTime << "ms";

    for (int i=0; i<numFiles; i++) {
        delete quillFile[i];
        QFile::remove(fileName[i]);
    }
}
