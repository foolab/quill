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

#include "test_utils.h"

#include <QtTest/QtTest>

void test_utils::analyze(QImage image1, QImage image2, int tolerance)
{
    QCOMPARE(image1.size(), image2.size());

    int redErrorCumulative = 0,
        greenErrorCumulative = 0,
        blueErrorCumulative = 0,
        pixelCount = image1.width() * image1.height(),
        badPixelCount = 0;

    int redErrorMin = 0, redErrorMax = 0,
        greenErrorMin = 0, greenErrorMax = 0,
        blueErrorMin = 0, blueErrorMax = 0,
        redErrorFreq[512], greenErrorFreq[512], blueErrorFreq[512];

    for (int a=0; a<512; a++)
        redErrorFreq[a] = greenErrorFreq[a] = blueErrorFreq[a] = 0;

    QImage image3(image1);
    image3.fill(qRgb(0, 0, 0));

    for (int x=0; x<image1.width(); x++)
        for (int y=0; y<image1.height(); y++) {

            QRgb pixel1 = image1.pixel(x, y),
                pixel2 = image2.pixel(x, y);

            QCOMPARE(qAlpha(pixel1), 255);
            QCOMPARE(qAlpha(pixel2), 255);

            int redError = qRed(pixel2) - qRed(pixel1);
            int greenError = qGreen(pixel2) - qGreen(pixel1);
            int blueError = qBlue(pixel2) - qBlue(pixel1);

            if (redError < redErrorMin) redErrorMin = redError;
            if (redError > redErrorMax) redErrorMax = redError;
            if (greenError < greenErrorMin) greenErrorMin = greenError;
            if (greenError > greenErrorMax) greenErrorMax = greenError;
            if (blueError < blueErrorMin) blueErrorMin = blueError;
            if (blueError > blueErrorMax) blueErrorMax = blueError;

            if (redError > tolerance || redError < -tolerance ||
                greenError > tolerance || greenError < -tolerance ||
                blueError > tolerance || blueError < -tolerance)
                badPixelCount++;

            redErrorFreq[redError + 256]++;
            greenErrorFreq[greenError + 256]++;
            blueErrorFreq[blueError + 256]++;

            redErrorCumulative += redError;
            greenErrorCumulative += greenError;
            blueErrorCumulative += blueError;

            image3.setPixel(x, y,
                            qRgb(128 + redError * 16, 128 + greenError * 16, 128 + blueError * 16));
        }

    qDebug() << "Red channel";
    qDebug() << "Largest errors :" << redErrorMin << "to" << redErrorMax;
    //    reportErrorFreq(redErrorFreq, redErrorMin, redErrorMax, pixelCount);

    qDebug() << "Systematic error :" << redErrorCumulative * 1.0 / pixelCount;

    qDebug() << "Green channel";
    qDebug() << "Largest errors :" << greenErrorMin << "to" << greenErrorMax;
    //    reportErrorFreq(greenErrorFreq, greenErrorMin, greenErrorMax, pixelCount);

    qDebug() << "Systematic error :" << greenErrorCumulative * 1.0 / pixelCount;

    qDebug() << "Blue channel";
    qDebug() << "Largest errors :" << blueErrorMin << "to" << blueErrorMax;
    //    reportErrorFreq(blueErrorFreq, blueErrorMin, blueErrorMax, image1.width() * image1.height());

    qDebug() << "Systematic error :" << blueErrorCumulative * 1.0 / pixelCount;

    qDebug() << "Bad pixels (one or more values differ by more than " << tolerance << "):" << badPixelCount << "(" << badPixelCount * 100.0 / pixelCount << "% of total)";

    // Very basic criteria

    QVERIFY(badPixelCount <= pixelCount / 20);
}

void test_utils::reportErrorFreq(int* freq, int min, int max, int total)
{
    for (int a=min; a<=max; a++)
        qDebug() << "Error" << a << ":" << freq[256+a] << "(" <<
            freq[256+a] * 100.0 / total << "% of total )";
}
