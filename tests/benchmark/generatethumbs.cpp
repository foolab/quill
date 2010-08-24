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

void generateThumbs(QString originalFileName, int n, QSize size, QSize minimumSize, QString mimeType, QString flavor)
{
    qDebug() << "Generating" << n << flavor << size.width() << "x" << size.height() << "thumbnails for" << originalFileName << "MIME" << mimeType;

    QEventLoop loop;
    QTime time;

    Quill::setTemporaryFilePath(QDir::homePath()+"/.config/quill/tmp/");
    Quill::setPreviewSize(0, size);
    Quill::setMinimumPreviewSize(0, minimumSize);
    Quill::setThumbnailDirectory(0, QDir::homePath()+"/.thumbnails/"+flavor);
    Quill::setThumbnailExtension("jpeg");
    Quill::setThumbnailCreationEnabled(false);

    int numFiles = n;

    Quill::setFileLimit(0, numFiles);

    QString fileName[numFiles];
    QuillFile *quillFile[numFiles];

    for (int i=0; i<numFiles; i++) {
        {   // Needed for the life of the QTemporaryFile
            QTemporaryFile file;
            file.setFileTemplate(QDir::homePath()+"/.config/quill/tmp/XXXXXX");
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
            qDebug("Error: not all images are loaded!");
            return;
        }

    qDebug() << "Initialize" << numFiles << "QuillFiles:"
             << initTime << "ms";

    qDebug() << "Set display levels of" << numFiles << "QuillFiles:"
             << displayLevelTime - initTime << "ms";

    qDebug() << "Use case generate" << numFiles << "thumbnails:"
             << finalTime << "ms";

    for (int i=0; i<numFiles; i++) {
        delete quillFile[i];
        QFile::remove(fileName[i]);
    }
}
