#include <unistd.h>
#include <QCoreApplication>
#include <QDebug>

#include "batchrotate.h"
#include "generatethumbs.h"
#include "loadthumbs.h"

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
    qDebug() << "-n  number of files, default 100";
    qDebug() << "-w  thumbnail width, default 128";
    qDebug() << "-h  thumbnail height, default 128";
    qDebug() << "-f  force thumbnail size";
}

int c;
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 3)
        help();
    else if ((QString(argv[1]) == "00") || (QString(argv[1]) == "rotate"))
        batchrotate(argv[2]);
    else if ((QString(argv[1]) == "01") || (QString(argv[1]) == "loadthumbs")) {
        QString fileName = argv[2];

        int n = 100, w = 128, h = 128;
        while ((c = getopt(argc, argv, "n:w:h:f")) != -1) {
            switch(c) {
            case 'n' :
                n = QString(optarg).toInt();
                break;
            case 'w' :
                w = QString(optarg).toInt();
                break;
            case 'h' :
                h = QString(optarg).toInt();
                break;
            }
        }

        QSize size(w, h);

        loadThumbs(fileName, n, size);
    }
    else if ((QString(argv[1]) == "02") || (QString(argv[1]) == "generatethumbs")) {
        QString fileName = argv[2];

        int n = 100, w = 128, h = 128;
        bool f = false;
        while ((c = getopt(argc, argv, "n:w:h:f")) != -1) {
            switch(c) {
            case 'n' :
                n = QString(optarg).toInt();
                break;
            case 'w' :
                w = QString(optarg).toInt();
                break;
            case 'h' :
                h = QString(optarg).toInt();
                break;
            case 'f' :
                f = true;
            }
        }

        QSize size(w, h);
        QSize minimumSize;
        if (f)
            minimumSize = size;

        generateThumbs(fileName, n, size, minimumSize);
    }
    else
        help();
}
