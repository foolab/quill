#include <QCoreApplication>
#include <QDebug>

#include "batchrotate.cpp"

void help()
{
    qDebug() << "Usage: benchmark [case] [filename] <options>";
    qDebug();
    qDebug() << "Cases:";
    qDebug() << "00 rotate         - Batch load/rotate/save";
    qDebug() << "01 loadthumbs     - Load multiple thumbnails";
    qDebug() << "02 generatethumbs - Generate multiple thumbnails for viewing";
    qDebug();
    qDebug() << "Options:";
    qDebug() << "-n  number of files";
    qDebug() << "-w  thumbnail width";
    qDebug() << "-h  thumbnail height";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 3)
        help();
    else if ((QString(argv[1]) == "00") || (QString(argv[1]) == "rotate")) {
        batchrotate(argv[2]);
    }
    else
        help();
}
