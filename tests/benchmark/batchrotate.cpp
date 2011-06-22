#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QTime>
#include <iostream>
#include <QTemporaryFile>

#include <Quill>
#include <QuillFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>

#include "../../src/strings.h"

void batchrotate(QString fileName)
{
    QEventLoop loop;

    QTime time;
    time.start();

    Quill::setDefaultTileSize(QSize(256, 256));

    QFile origFile(fileName);
    origFile.open(QIODevice::ReadOnly);
    QByteArray buf = origFile.readAll();
    origFile.close();

    QTemporaryFile file;
    file.open();
    file.write(buf);
    file.flush();

    QuillFile quillFile(file.fileName(), Strings::jpg);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Rotate);
    filter->setOption(QuillImageFilter::Angle, QVariant(90));

    quillFile.runFilter(filter);

    QObject::connect(&quillFile, SIGNAL(saved()), &loop, SLOT(quit()));

    quillFile.save();

    loop.exec();

    std::cout << "Use case batch rotate/save: " << time.elapsed() << "ms" << "\n";
}
