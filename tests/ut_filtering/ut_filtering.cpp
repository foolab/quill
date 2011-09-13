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

#include <QDebug>
#include <QtTest/QtTest>
#include <QImage>
#include <QSignalSpy>
#include <QMetaType>
#include <QTemporaryFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>
#include <Quill>

#include "quillerror.h"
#include "core.h"
#include "file.h"
#include "ut_filtering.h"
#include "unittests.h"
#include "quillfile.h"
#include "../../src/strings.h"

ut_filtering::ut_filtering()
{
}

void ut_filtering::initTestCase()
{
    m_tmpFileList.clear();
}

void ut_filtering::cleanupTestCase()
{
    foreach (QTemporaryFile *pTmpFile, m_tmpFileList) {
	pTmpFile->setAutoRemove(true);
	delete pTmpFile;
    }
}

void ut_filtering::init()
{
    Quill::initTestingMode();
}

void ut_filtering::cleanup()
{
    Quill::cleanup();
}

void ut_filtering::myReleaseAndWait()
{
    for (int r=0; r<100; r++)
	Quill::releaseAndWait();
}

QTemporaryFile* ut_filtering::createTempFile(QString fileName)
{
    QTemporaryFile *pTmpFile = new QTemporaryFile();
    QFile origFile(fileName);
    origFile.open(QIODevice::ReadOnly);
    QByteArray buf = origFile.readAll();
    if (buf.length() <= 0)
	return NULL;
    origFile.close();

    pTmpFile->open();
    pTmpFile->write(buf);
    pTmpFile->flush();
    pTmpFile->setAutoRemove(false);

    if (!m_tmpFileList.contains(pTmpFile))
	m_tmpFileList.append(pTmpFile);

    return pTmpFile;
}

void ut_filtering::createFreeRotate_BrightnessContrastFilters(
	int nRotateAngle, int nBrightness, int nContrast,
	QuillImageFilter **f_freerotate, QuillImageFilter **f_brightness)
{
    *f_freerotate =
	    QuillImageFilterFactory::createImageFilter(
		    QuillImageFilter::Name_FreeRotate);
    (*f_freerotate)->setOption(QuillImageFilter::Angle, QVariant(nRotateAngle));
    *f_brightness =
	    QuillImageFilterFactory::createImageFilter(
		    QuillImageFilter::Name_BrightnessContrast);
    (*f_brightness)->setOption(QuillImageFilter::Brightness, nBrightness);
    (*f_brightness)->setOption(QuillImageFilter::Contrast,   nContrast);
}

void ut_filtering::createRER_CropFilters(
	QRect cropRect,
	QuillImageFilter **f_rer, QuillImageFilter **f_crop)
{
    *f_rer =
	QuillImageFilterFactory::createImageFilter("org.maemo.red-eye-detection");

    *f_crop =
	QuillImageFilterFactory::createImageFilter("org.maemo.crop");
    QVERIFY((*f_crop)->setOption(QuillImageFilter::CropRectangle, cropRect));
}


QTemporaryFile* ut_filtering::rotateAndAdjust(QString fileName,
	int nRepeat, int nRotateAngle, int nBrightness, int nContrast, bool bRotateFirst)
{
    QuillImageFilter *f_freerotate;
    QuillImageFilter *f_brightness;

    QTemporaryFile *tmpFile1 = createTempFile(fileName);
    QuillFile *quillFile1 = new QuillFile(tmpFile1->fileName(), Strings::jpg);

    for (int r=0; r<nRepeat; r++) {
	createFreeRotate_BrightnessContrastFilters(nRotateAngle,
						   ((r%2)*2-1)*nBrightness,
						   ((r%2)*2-1)*nContrast,
						   &f_freerotate, &f_brightness);

	if (bRotateFirst)
	    quillFile1->runFilter(f_freerotate);
	else
	    quillFile1->runFilter(f_brightness);

	quillFile1->save();
	myReleaseAndWait();

	if (bRotateFirst)
	    quillFile1->runFilter(f_brightness);
	else
	    quillFile1->runFilter(f_freerotate);

	quillFile1->save();
	myReleaseAndWait();
    }

    tmpFile1->flush();
    tmpFile1->close();

    delete quillFile1;
    return tmpFile1;
}


QTemporaryFile* ut_filtering::rotateAndAdjustWithSave(QString fileName,
	int nRepeat, int nRotateAngle, int nBrightness, int nContrast, bool bRotateFirst)
{
    QuillImageFilter *f_freerotate;
    QuillImageFilter *f_brightness;

    QTemporaryFile *tmpFile1a = createTempFile(fileName);
    QuillFile *quillFile1a = NULL;
    QTemporaryFile *tmpFile1b = NULL;
    QuillFile *quillFile1b = NULL;

    for (int r=0; r<nRepeat; r++) {
	// Create filters
	createFreeRotate_BrightnessContrastFilters(nRotateAngle,
						   ((r%2)*2-1)*nBrightness,
						   ((r%2)*2-1)*nContrast,
						   &f_freerotate, &f_brightness);

	// Create image 'a' from file
	if (quillFile1a) {
	    quillFile1a->remove();
	    delete quillFile1a;
	}
	quillFile1a = new QuillFile(tmpFile1a->fileName(), Strings::jpg);

	// Filter & save
	if (bRotateFirst)
	    quillFile1a->runFilter(f_freerotate);
	else
	    quillFile1a->runFilter(f_brightness);

	quillFile1a->save();
	myReleaseAndWait();

	tmpFile1a->flush();
	tmpFile1a->close();

	// Create file 'b' by copying file where image 'a' is written to
	//delete tmpFile1b;
	tmpFile1b = createTempFile(tmpFile1a->fileName());

	// Create image 'b' from file
	if (quillFile1b) {
	    quillFile1b->remove();
	    delete quillFile1b;
	}
	quillFile1b = new QuillFile(tmpFile1b->fileName(), Strings::jpg);

	// Filter & save
	if (bRotateFirst)
	    quillFile1b->runFilter(f_brightness);
	else
	    quillFile1b->runFilter(f_freerotate);

	quillFile1b->save();
	myReleaseAndWait();

	tmpFile1b->flush();
	tmpFile1b->close();

	// Create file 'a' by copying file where image 'b' is written to
	//delete tmpFile1a;
	tmpFile1a = createTempFile(tmpFile1b->fileName());
    }

    //delete tmpFile1b;
    quillFile1b->remove();
    delete quillFile1b;
    delete quillFile1a;

    return tmpFile1a;
}


void ut_filtering::testFreerotateWithBrightnessContrast_noSaving()
{
    QString fileName = "/usr/share/libquill-tests/images/redeye.jpg";

    int nAngle = -15;
    int nRepeat = 4;
    int nBrightness = 20;
    int nContrast = -20;

    Quill::setDefaultTileSize(QSize(64, 64));

    // Rotate first:
    QTemporaryFile *tmpFile1 = rotateAndAdjust(
	    fileName, nRepeat, nAngle, nBrightness, nContrast, true);

    // Adjust first:
    QTemporaryFile *tmpFile2 = rotateAndAdjust(
	    fileName, nRepeat, nAngle, nBrightness, nContrast, false);

    // Created images needed comparing pixel data
    QImage ima1(tmpFile1->fileName());
    QVERIFY(!ima1.isNull());
    QImage ima2(tmpFile2->fileName());
    QVERIFY(!ima2.isNull());

    // Comparisons:
    double dPSNR = Unittests::getPSNR(ima1, ima2);
    qDebug() << dPSNR << "dB";
    QVERIFY(dPSNR > 30);
}


void ut_filtering::testFreerotateWithBrightnessContrast_savingInBetween()
{
    QString fileName = "/usr/share/libquill-tests/images/redeye.jpg";

    int nAngle = -15;
    int nRepeat = 4;
    int nBrightness = 20;
    int nContrast = -20;

    Quill::setDefaultTileSize(QSize(64, 64));

    // Rotate first, with saving:
    QTemporaryFile *tmpFile1a = rotateAndAdjustWithSave(
	    fileName, nRepeat, nAngle, nBrightness, nContrast, true);

    // Adjust first, with saving:
    QTemporaryFile *tmpFile2a = rotateAndAdjustWithSave(
	    fileName, nRepeat, nAngle, nBrightness, nContrast, false);


    // Created images needed comparing pixel data
    QImage ima1(tmpFile1a->fileName());
    QVERIFY(!ima1.isNull());
    QImage ima2(tmpFile2a->fileName());
    QVERIFY(!ima2.isNull());

    // Comparisons:
    double dPSNR = Unittests::getPSNR(ima1, ima2);
    qDebug() << dPSNR << "dB";
    QVERIFY(dPSNR > 30);
}


void ut_filtering::testRedEyeRemovalWithCrop()
{

    QString fileName = "/usr/share/libquill-tests/images/redeye01.JPG";

    int nRepeat = 4;
    QuillImageFilter *f_rer;
    QuillImageFilter *f_crop;

    Quill::setDefaultTileSize(QSize(128, 128));

    // RER first:
    QTemporaryFile *tmpFile1 = createTempFile(fileName);
    QVERIFY(tmpFile1);
    {
	QuillFile quillFile1(tmpFile1->fileName(), Strings::jpg);

	for (int r=0; r<nRepeat; r++) {
	    QImage tempImage(quillFile1.fileName());
	    QRect cropRect = QRect(QPoint(10, 10), tempImage.size()-QSize(20, 20));
	    createRER_CropFilters(cropRect,
				  &f_rer, &f_crop);

	    quillFile1.runFilter(f_rer);
	    quillFile1.save();
	    myReleaseAndWait();

	    quillFile1.runFilter(f_crop);
	    quillFile1.save();
	    myReleaseAndWait();
	}
	quillFile1.remove();
    }
    // Crop first:
    QTemporaryFile *tmpFile2 = createTempFile(fileName);
    {
	QuillFile quillFile2(tmpFile2->fileName(), Strings::jpg);
	for (int r=0; r<nRepeat; r++) {
	    QImage tempImage(quillFile2.fileName());
	    QRect cropRect = QRect(QPoint(10, 10), tempImage.size()-QSize(20, 20));
	    createRER_CropFilters(cropRect,
				  &f_rer, &f_crop);

	    quillFile2.runFilter(f_crop);
	    quillFile2.save();
	    myReleaseAndWait();

	    quillFile2.runFilter(f_rer);
	    quillFile2.save();
	    myReleaseAndWait();
	}
	quillFile2.remove();
    }

    // Compare & cleanup:
    tmpFile1->flush();
    tmpFile1->close();
    tmpFile1->open();
    QByteArray buf1 = tmpFile1->readAll();
    tmpFile1->close();

    tmpFile2->flush();
    tmpFile2->close();
    tmpFile2->open();
    QByteArray buf2 = tmpFile2->readAll();
    tmpFile2->close();

    // Should be identical (= same size after compression)
    QCOMPARE(qChecksum(buf1.data(),buf1.capacity()), qChecksum(buf2.data(),buf2.capacity()));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_filtering test;
    return QTest::qExec( &test, argc, argv );
}

