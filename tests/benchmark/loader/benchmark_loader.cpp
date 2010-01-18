#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QTime>
#include <QDebug>

#include <Quill>
#include <QuillFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>

#include "timelistener.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv); // Without this, QEventLoop will not work
    QEventLoop loop;

    QTime time;
    time.start();

    for(int i = 0; i<argc; i++)
        qDebug()<<"the arg is: "<<argv[i];

    Quill::setPreviewSize(0, QSize(320, 200));
    Quill::setDefaultTileSize(QSize(256, 256));

    TimeListener *listener = new TimeListener();

    QuillFile *file = new QuillFile("input/benchmark12.jpg", "jpg");
    file->setDisplayLevel(1);
    file->setViewPort(QRect(QPoint(0, 0), file->fullImageSize()));

    QObject::connect(file, SIGNAL(imageAvailable(const QuillImageList)),
                     listener, SLOT(timer()));

    loop.exec();
}
