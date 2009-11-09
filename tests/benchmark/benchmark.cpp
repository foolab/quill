#include <QCoreApplication>
#include <QEventLoop>
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

    Quill *quill = new Quill(QSize(100, 100));

    quill->setDefaultTileSize(QSize(256, 256));

    QuillFile *file = quill->file("input/benchmark12.jpg");

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
