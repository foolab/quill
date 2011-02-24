#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QTime>
#include <QDebug>

#include <Quill>
#include <QuillFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "../../src/strings.h"

void tiling(QString fileName, QSize size)
{
    QEventLoop loop;

    QTime time;
    time.start();

    Quill::setPreviewSize(0, QSize(320, 200));
    Quill::setDefaultTileSize(QSize(256, 256));

    QuillFile *file = new QuillFile(fileName, Strings::jpg);
    file->setDisplayLevel(1);
    file->setViewPort(QRect(QPoint((file->fullImageSize().width() - size.width()) / 2,
                                   (file->fullImageSize().height() - size.height()) / 2),
                            size));

    QObject::connect(file, SIGNAL(imageAvailable(const QuillImageList)),
                     &loop, SLOT(quit()));

    qDebug() << "Getting 256x256 tiles inside" << file->viewPort();

    int tileCount = 0;

    do {
        loop.exec();

        if (tileCount > 0)
            qDebug() << "Time elapsed for tile" << time.elapsed();
        else
            qDebug() << "Time elapsed for setup and preview:" << time.elapsed();

        tileCount++;

        time.restart();

    } while (Quill::isCalculationInProgress());
}
