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

void createLoadFilters()
{
    QTime time;
    time.start();

    QSize a;

    QuillImageFilter *filter[100];
    for (int i=0; i<100; i++) {
        filter[i] = QuillImageFilterFactory::createImageFilter("org.maemo.load");
        filter[i]->setOption(QuillImageFilter::FileName, "input/benchmark12.jpg");
        a = filter[i]->newFullImageSize(QSize());
    }
    qDebug() << "Handling 100 QuillImageFilters: " << time.elapsed() << "ms";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv); // Without this, QEventLoop will not work
    QEventLoop loop;

    //    createLoadFilters();

    QTime time;

    for(int i = 0; i<argc; i++)
        qDebug()<<"the arg is: "<<argv[i];

    Quill::setTemporaryFilePath(QDir::homePath()+"/.config/quill/tmp/");
    Quill::setDefaultTileSize(QSize(256, 256));
    Quill::setPreviewSize(0, QSize(128, 128));
    Quill::setDBusThumbnailingEnabled(false);

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
            file[i].setFileTemplate(QDir::homePath()+"/.config/quill/tmp/XXXXXX.jpeg");
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
            return 0;
        }

    qDebug() << "Initialize" << numFiles << "QuillFiles:"
             << initTime << "ms";

    qDebug() << "Set display levels of" << numFiles << "QuillFiles:"
             << displayLevelTime - initTime << "ms";

    qDebug() << "Use case generate" << numFiles << "thumbnails:"
             << finalTime << "ms";

    for (int i=0; i<numFiles; i++)
        delete quillFile[i];
}
