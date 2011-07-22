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

#include <QtTest/QtTest>
#include <QSize>
#include <QRect>
#include <QFont>
#include <QPen>
#include <QPolygon>
#include "historyxml.h"

#include "unittests.h"
#include "ut_xml.h"

ut_xml::ut_xml()
{
}

void ut_xml::initTestCase()
{
}

void ut_xml::cleanupTestCase()
{
}

void ut_xml::testPointVariant()
{
    QByteArray dump;
    QXmlStreamWriter writer(&dump);

    QPoint point(4, 3);

    HistoryXml::writeComplexType(point, &writer);

    QXmlStreamReader reader(dump);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QString("QPoint"));

    QCOMPARE(HistoryXml::recoverComplexType(QVariant::Point, &reader).toPoint(),
             point);
    reader.readNext();
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Point, &reader));
    //test default case
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Map, &reader));
    //test false case
    QByteArray dump1;
    QXmlStreamWriter writer1(&dump1);
    QSize size(4, 3);

    HistoryXml::writeComplexType(size, &writer1);
    QXmlStreamReader reader1(dump1);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Point, &reader1));

}

void ut_xml::testSizeVariant()
{
    QByteArray dump;
    QXmlStreamWriter writer(&dump);

    QSize size(4, 3);

    HistoryXml::writeComplexType(size, &writer);

    QXmlStreamReader reader(dump);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QString("QSize"));

    QCOMPARE(HistoryXml::recoverComplexType(QVariant::Size, &reader).toSize(),
             size);
    reader.readNext();
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Size, &reader));
    //test false case
    QByteArray dump1;
    QXmlStreamWriter writer1(&dump1);
    QPoint point(4, 3);

    HistoryXml::writeComplexType(point, &writer1);
    QXmlStreamReader reader1(dump1);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Size, &reader1));

}

void ut_xml::testRectVariant()
{
    QByteArray dump;
    QXmlStreamWriter writer(&dump);

    QRect rect(1, 2, 3, 4);

    HistoryXml::writeComplexType(rect, &writer);

    QXmlStreamReader reader(dump);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QString("QRect"));

    QCOMPARE(HistoryXml::recoverComplexType(QVariant::Rect, &reader).toRect(),
             rect);
    reader.readNext();
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Rect, &reader));
    //test false case
    QByteArray dump1;
    QXmlStreamWriter writer1(&dump1);
    QSize point(4, 3);

    HistoryXml::writeComplexType(point, &writer1);
    QXmlStreamReader reader1(dump1);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Rect, &reader1));
}

void ut_xml::testColorVariant()
{
    QByteArray dump;
    QXmlStreamWriter writer(&dump);

    QColor color(255, 128, 96, 64);

    HistoryXml::writeComplexType(color, &writer);

    QXmlStreamReader reader(dump);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QString("QRgba"));

    QCOMPARE(HistoryXml::recoverComplexType(QVariant::Color, &reader).value<QColor>(),
             color);
    reader.readNext();
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Color, &reader));
    //test false case
    QByteArray dump1;
    QXmlStreamWriter writer1(&dump1);
    QSize point(4, 3);

    HistoryXml::writeComplexType(point, &writer1);
    QXmlStreamReader reader1(dump1);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader1.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Color, &reader1));
}

void ut_xml::testFontVariant()
{
    QByteArray dump;
    QXmlStreamWriter writer(&dump);

    QFont font;

    HistoryXml::writeComplexType(font, &writer);

    QXmlStreamReader reader(dump);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QString("QFont"));

    QCOMPARE(HistoryXml::recoverComplexType(QVariant::Font, &reader).value<QFont>(),
             font);
    reader.readNext();
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Font, &reader));
}


void ut_xml::testPenVariant()
{
    QByteArray dump;
    QXmlStreamWriter writer(&dump);

    QPen pen;

    HistoryXml::writeComplexType(pen, &writer);

    QXmlStreamReader reader(dump);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QString("QPen"));

    QCOMPARE(HistoryXml::recoverComplexType(QVariant::Pen, &reader).value<QPen>(),
             pen);
    reader.readNext();
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Pen, &reader));
}

void ut_xml::testPolygonVariant()
{
    QByteArray dump;
    QXmlStreamWriter writer(&dump);

    QPolygon polygon;

    HistoryXml::writeComplexType(polygon, &writer);

    QXmlStreamReader reader(dump);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartDocument);
    QCOMPARE(reader.readNext(), QXmlStreamReader::StartElement);
    QCOMPARE(reader.name().toString(), QString("QPolygon"));

    QCOMPARE(HistoryXml::recoverComplexType(QVariant::Polygon, &reader).value<QPolygon>(),
             polygon);
    reader.readNext();
    QCOMPARE(QVariant(),HistoryXml::recoverComplexType(QVariant::Polygon, &reader));
}

void ut_xml::testRecoverVariant()
{
    bool flag1 = false;
    bool flag2 = true;
    QCOMPARE(QVariant(1),HistoryXml::recoverVariant(QVariant::Int, QString("1")));
    QCOMPARE(QVariant(1.5),HistoryXml::recoverVariant(QVariant::Double, QString("1.5")));
    QCOMPARE(QVariant("Test"),HistoryXml::recoverVariant(QVariant::String, QString("Test")));
    QCOMPARE(QVariant(flag1),HistoryXml::recoverVariant(QVariant::Bool, QString("false")));
    QCOMPARE(QVariant(flag2),HistoryXml::recoverVariant(QVariant::Bool, QString("true")));
    //testing default case
    QCOMPARE(QVariant(),HistoryXml::recoverVariant(QVariant::Map, QString("quill")));
}

void ut_xml::testEmptyDocument()
{
    // we don't need a core in this one
    QList<File*> list = HistoryXml::decode(QByteArray());

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlVersionOnly()
{
    QList<File*> list =
        HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlVersionDtd()
{
    QList<File*> list =
        HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                      <!DOCTYPE QuillEditHistory>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlInvalidTopElement()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Quill/>"));

    QVERIFY(list.isEmpty());
}


void ut_xml::testXmlEmptyCore()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core/>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlBadCoreContents()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><Stack/></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlEmptyStack()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack/></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlBadStackContents()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack><Filter/>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlBadFileContents()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack>\
                                  <File>jussi.jpg</File>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlNoOriginalFile()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack>\
                                  <File name=\"jussi.jpg\"/>\
                                  <TargetIndex>2</TargetIndex>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlBadOriginalFileContents()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack>\
                                  <File name=\"jussi.jpg\"/>\
                                  <OriginalFile>jussi.jpg</OriginalFile>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlNoTargetIndex()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack>\
                                  <File name=\"jussi.jpg\"/>\
                                  <OriginalFile name=\"jussi_original.jpg\"/>\
                                  <SavedIndex>2</SavedIndex>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlEmptyTargetIndex()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack>\
                                  <File name=\"jussi.jpg\"/>\
                                  <OriginalFile name=\"jussi_original.jpg\"/>\
                                  <TargetIndex/>\
                                  <SavedIndex>2</SavedIndex>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}


void ut_xml::testXmlNoSavedIndex()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack>\
                                  <File name=\"jussi.jpg\"/>\
                                  <OriginalFile name=\"jussi_original.jpg\"/>\
                                  <TargetIndex>2</TargetIndex>\
                                  <Filter/>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}

void ut_xml::testXmlEmptySavedIndex()
{
    QList<File*> list =
    HistoryXml::decode(QByteArray("<?xml version=\"1.0\"?>\
                                  <!DOCTYPE QuillEditHistory>\
                                  <Core><QuillUndoStack>\
                                  <File name=\"jussi.jpg\"/>\
                                  <OriginalFile name=\"jussi_original.jpg\"/>\
                                  <TargetIndex>2</TargetIndex>\
                                  <SavedIndex/>\
                                  <Filter/>\
                                  </QuillUndoStack></Core>"));

    QVERIFY(list.isEmpty());
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_xml test;
    return QTest::qExec( &test, argc, argv );
}
