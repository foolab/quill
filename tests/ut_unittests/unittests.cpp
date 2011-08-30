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

#include <stdlib.h>
#include "unittests.h"
#include <QImage>
#include <QtTest/QtTest>
#include <QDebug>

int palette16[16][3] =
{{  0,   0,     0}, {132,   0,   0}, {  0, 132,   0}, {132, 132,   0},
 {  0,   0,   132}, {132,   0, 132}, {  0, 132, 132}, {132, 132, 132},
 {  198, 198, 198}, {255,   0,   0}, {  0, 255,   0}, {255, 255,   0},
 {  0,   0,   255}, {255,   0, 255}, {  0, 255, 255}, {255, 255, 255}};


QImage Unittests::generatePaletteImage()
{
    QImage image = QImage(QSize(8, 2), QImage::Format_RGB32);
    for (int p=0; p<16; p++)
    {
        image.setPixel(p%8, p/8, qRgb(palette16[p][0],
                                      palette16[p][1],
                                      palette16[p][2]));
    }
    return image;
}

QImage Unittests::generatePaletteImage(int minValue, int maxValue)
{
    QImage image = QImage(QSize(8, 2), QImage::Format_RGB32);
    for (int p=0; p<16; p++)
    {
        image.setPixel(p%8, p/8, qRgb(
                palette16[p][0] * ((maxValue - minValue)/255.0) + minValue,
                palette16[p][1] * ((maxValue - minValue)/255.0) + minValue,
                palette16[p][2] * ((maxValue - minValue)/255.0) + minValue));
    }
    return image;
}

void Unittests::compareReal(qreal real1, qreal real2)
{
    if (fabs(real1 - real2) > .00001)
    {
        qDebug() << "Error: Compared values are not the same!";
        qDebug() << "Actual" << real1 << "expected" << real2;
        QFAIL("Compared values are not the same!");
    }
}

void Unittests::fuzzyCompareRgb(QRgb rgb1, QRgb rgb2)
{
    if (abs(qRed(rgb1) - qRed(rgb2)) > 1)
    {
        qDebug() << "Error: Red components are not the same!";
        qDebug() << "Actual" << qRed(rgb1) << "expected" << qRed(rgb2);
        QFAIL("Red components are not the same!");
    }
    if (abs(qGreen(rgb1) - qGreen(rgb2)) > 1)
    {
        qDebug() << "Error: Green components are not the same!";
        qDebug() << "Actual" << qGreen(rgb1) << "expected" << qGreen(rgb2);
        QFAIL("Green components are not the same!");
    }
    if (abs(qBlue(rgb1) - qBlue(rgb2)) > 1)
    {
        qDebug() << "Error: Blue components are not the same!";
        qDebug() << "Actual" << qBlue(rgb1) << "expected" << qBlue(rgb2);
        QFAIL("Blue components are not the same!");
    }
}

bool Unittests::compareImage(QImage image1, QImage image2)
{
    if (image1.convertToFormat(QImage::Format_RGB32) ==
        image2.convertToFormat(QImage::Format_RGB32))
        return true;

    if (image1.size() != image2.size())
    {
        qDebug() << "Error: image sizes do not match!";
        qDebug() << "Actual:" << image1.size() << "Expected:" << image2.size();

        return false;
    }

    QRgb *pixel1 = (QRgb*) image1.bits(),
         *pixel2 = (QRgb*) image2.bits();

    qDebug() << "Image data follows:";

    for (int i=0; i<image1.numBytes()/4; i++)
        qDebug() << QString::number(pixel1[i], 16)
                 << QString::number(pixel2[i], 16);

    return false;
}

double Unittests::getPSNR(QImage image1, QImage image2)
{
    if (image1.size() != image2.size())
    {
	qDebug() << "Error: image sizes do not match!";
	qDebug() << "Actual:" << image1.size() << "Expected:" << image2.size();

	return -1.0;
    }

    QRgb *pixel1 = (QRgb*) image1.bits(),
	 *pixel2 = (QRgb*) image2.bits();

    int nPixels = image1.size().width()*image1.size().height();
    long nSqrSum = 0;
    for (int i=0; i<nPixels; i++) {
	int tmp1 = (int)(0.3*qRed(pixel1[i]) + 0.59*qGreen(pixel1[i]) + 0.11*qBlue(pixel1[i]) + 0.5);
	int tmp2 = (int)(0.3*qRed(pixel2[i]) + 0.59*qGreen(pixel2[i]) + 0.11*qBlue(pixel2[i]) + 0.5);
	nSqrSum += (tmp1-tmp2) * (tmp1-tmp2);
    }
    double dPSNR;
    double MSE = nSqrSum/nPixels;
    if (MSE > 0)
	dPSNR = 10*log10(255*255 / MSE);
    else
	dPSNR = 100;

    return dPSNR;
}

bool Unittests::isRoot()
{
    return QString(getenv("USER")) == QString("root");
}
