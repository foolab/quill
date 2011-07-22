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
#ifndef __QUILL_LOGGER_H__
#define __QUILL_LOGGER_H__
#include <QObject>
#include <QFile>

#ifdef QT_NO_DEBUG_OUTPUT
# define QUILL_LOG(x,y) ((void)0)
#else
# define QUILL_LOG(x,y) Logger::log(x,y)

class QSize;

class Logger : public QObject
{Q_OBJECT

friend class ut_logger;

public:
    static const QLatin1String Module_Quill;
    static const QLatin1String Module_QuillFile;
    static const QLatin1String Module_File;
    static const QLatin1String Module_Core;
    static const QLatin1String Module_Stack;
    static const QLatin1String Module_Scheduler;
    static const QLatin1String Module_ThreadManager;
    static const QLatin1String Module_DBusThumbnailer;

    Logger();

    static void log(const QString &module,
                    const QString &logInfo);

    static QString intToString(const int value);

    static QString qsizeToString(const QSize &size);

    static QString boolToString(const bool value);

private:
    static bool existFlag;
    static QFile data;
    static bool firstTimeFlag;
};

#endif
#endif
