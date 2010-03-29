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

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv); // Without this, QEventLoop will not work
    QEventLoop loop;

    QTime time;

    for(int i = 0; i<argc; i++)
        qDebug()<<"the arg is: "<<argv[i];

    Quill::setTemporaryFilePath(QDir::homePath()+"/.config/quill/tmp/");
    Quill::setDefaultTileSize(QSize(256, 256));
    Quill::setPreviewSize(0, QSize(128, 128));

    int numFiles = QString(argv[1]).toInt();

    Quill::setFileLimit(0, numFiles);

    QTemporaryFile file[numFiles];
    QString fileName[numFiles];
    QuillFile *quillFile[numFiles];

    {
        QFile origFile("input/benchmark12.jpg");
        origFile.open(QIODevice::ReadOnly);
        QByteArray buf = origFile.readAll();
        origFile.close();

        for (int i=0; i<numFiles; i++) {
            file[i].open();
            fileName[i] = file[i].fileName();
            file[i].write(buf);
            file[i].flush();
        }
    }

    time.start();

    for (int i=0; i<numFiles; i++) {
        quillFile[i] = new QuillFile(fileName[i], "jpg");
        QObject::connect(quillFile[i], SIGNAL(imageAvailable(const QuillImageList)),
                         &loop, SLOT(quit()));
    }

    for (int i=0; i<numFiles; i++)
        quillFile[i]->setDisplayLevel(0);

    while (Quill::isCalculationInProgress())
        loop.exec();

    int finalTime = time.elapsed();

    for (int i=0; i<numFiles; i++)
        if (quillFile[i]->image(0).isNull()) {
            qDebug("Error: not all images are loaded!");
            return 0;
        }

    qDebug() << "Use case generate" << numFiles << "thumbnails:"
             << finalTime << "ms";

    for (int i=0; i<numFiles; i++)
        delete quillFile[i];
}
