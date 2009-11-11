#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QTime>
#include <QDebug>

#include <Quill>
#include <QuillFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv); // Without this, QEventLoop will not work
    QEventLoop loop;

    QTime time;
    time.start();

    for(int i = 0; i<argc; i++)
        qDebug()<<"the arg is: "<<argv[i];
    Quill *quill = new Quill(QSize(100, 100));

    if(QString(argv[2]) =="t")
        quill->setTemporaryFilePath();
    quill->setDefaultTileSize(QSize(256, 256));

    QuillFile *file = quill->file("input/benchmark12.jpg", "jpg");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("Rotate");
    filter->setOption(QuillImageFilter::Angle, QVariant(90));

    file->runFilter(filter);

    QObject::connect(file, SIGNAL(saved()), &loop, SLOT(quit()));

    file->save();

    loop.exec();

    qDebug() << "Use case batch rotate/save:" << time.elapsed() << "ms";
    delete quill;
}
