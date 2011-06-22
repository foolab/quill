#include <unistd.h>
#include <QCoreApplication>
#include <QuillImageFilter>
#include <iostream>
#include "batchrotate.h"
#include "generatethumbs.h"
#include "loadthumbs.h"
#include "tiling.h"
#include "autofix.h"
#include "straighten.h"
#include "redeye.h"
#include "../../src/strings.h"

void help()
{
    std::cout << "Usage: benchmark [case] [filename] <options>\n";
    std::cout << "\n";
    std::cout << "Cases:";
    std::cout << "00 rotate         - Batch load/rotate/save\n";
    std::cout << "01 loadthumbs     - Load multiple thumbnails\n";
    std::cout << "02 generatethumbs - Generate multiple thumbnails for viewing\n";
    std::cout << "03 tiling         - Load tiles, use -w and -h for the size of the interest area\n";
    std::cout << "04 autofix        - Thumbnail response for Autofix edit\n";
    std::cout << "05 straighten     - Thumbnail responses for Straighten edit\n";
    std::cout << "06 redeye         - Thumbnail response for Red eye removal\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "-n  number of files, default 100\n";
    std::cout << "-w  thumbnail width, default 128\n";
    std::cout << "-h  thumbnail height, default 128\n";
    std::cout << "-f  force thumbnail size\n";
    std::cout << "-m  mime type, default image/jpeg\n";
    std::cout << "-d  D-Bus thumbnailing flavor name, default grid\n";
    std::cout << "-x  Effect centerpoint X, full-image coords (red eye removal)\n";
    std::cout << "-y  Effect centerpoint Y, full-image coords (red eye removal)\n";
    std::cout << "-t  Effect tolerance radius, full-image coords (red eye removal)\n";
}

int c;
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // Initialize the filter plugin framework here so that it will not
    // disturb the benchmark
    QuillImageFilter initPluginFramework("invalid");

    if (argc < 3)
        help();
    else if ((QString(argv[1]) == "00") || (QString(argv[1]) == "rotate"))
        batchrotate(argv[2]);
    else if ((QString(argv[1]) == "01") || (QString(argv[1]) == "loadthumbs")) {
        QString fileName = argv[2];

        int n = 100, w = 128, h = 128;
        while ((c = getopt(argc, argv, "n:w:h:")) != -1) {
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
        QString m = Strings::jpegMimeType, d = "grid";
        while ((c = getopt(argc, argv, "n:w:h:fm:d:")) != -1) {
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
            case 'd' :
                d = QString(optarg);
                break;
            case 'm':
                m = QString(optarg);
                break;
            case 'f':
                f = true;
                break;
            }
        }

        QSize size(w, h);
        QSize minimumSize;
        if (f)
            minimumSize = size;

        generateThumbs(fileName, n, size, minimumSize, m, d);
    }
    else if ((QString(argv[1]) == "03") || (QString(argv[1]) == "tiling")) {
        QString fileName = argv[2];

        int w = 800, h = 480;
        while ((c = getopt(argc, argv, "w:h:")) != -1) {
            switch(c) {
            case 'w' :
                w = QString(optarg).toInt();
                break;
            case 'h' :
                h = QString(optarg).toInt();
                break;
            }
        }

        QSize size(w, h);

        tiling(fileName, size);
    }
    else if ((QString(argv[1]) == "04") || (QString(argv[1]) == "autofix")) {
        QString fileName = argv[2];

        int n = 100, w = 128, h = 128;
        QString m = Strings::jpegMimeType, d = "grid";
        while ((c = getopt(argc, argv, "n:w:h:")) != -1) {
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

        autofix(fileName, n, size);
    }
    else if ((QString(argv[1]) == "05") || (QString(argv[1]) == "straighten")) {
        QString fileName = argv[2];

        int n = 100, w = 128, h = 128;
        QString m = Strings::jpegMimeType, d = "grid";
        while ((c = getopt(argc, argv, "n:w:h:")) != -1) {
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

        straighten(fileName, n, size);
    }
    else if ((QString(argv[1]) == "06") || (QString(argv[1]) == "redeye")) {
        QString fileName = argv[2];

        int n = 100, w = 128, h = 128, x = 0, y = 0, t = 150;
        QString m = Strings::jpegMimeType, d = "grid";
        while ((c = getopt(argc, argv, "n:w:h:x:y:t:")) != -1) {
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
            case 'x' :
                x = QString(optarg).toInt();
                break;
            case 'y':
                y = QString(optarg).toInt();
                break;
            case 't':
                t = QString(optarg).toInt();
                break;
            }
        }
        redeye(fileName, n, QSize(w, h), QPoint(x, y), t);
    }

    else
        help();
}
