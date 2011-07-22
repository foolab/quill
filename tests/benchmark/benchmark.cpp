/****************************************************************************
**
** Copyright (C) 2009-11 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Pekka Marjola <pekka.marjola@nokia.com>
**
** This file is part of the Quill package.
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

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
