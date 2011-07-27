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

/*!
  \class ThreadManager

  \brief Responsible for running the worker thread on the background.

Contains no explicit thread operations on its own, threading is done
by the QFuture and QFutureWatcher classes from Qt 4.5, which use
QThreadPool on the background.
 */

#ifndef THREADMANAGER_H
#define THREADMANAGER_H
#include <QObject>
#include "quill.h"

class QuillImage;
class Task;
class Core;
class QEventLoop;
class ThreadManager;
class QuillImageFilter;
class BackgroundThread;

class ThreadManager : public QObject
{
Q_OBJECT

public:
    ThreadManager(Quill::ThreadingMode mode = Quill::ThreadingNormal);

    ~ThreadManager();

    /*!
      If the thread manager is currently running a task in the background.
     */

    bool isRunning() const;

    /*!
      If the thread manager allows to delete the filter in question
      (meaning that the filter is currently being run.)
     */

    bool allowDelete(QuillImageFilter *filter) const;

    /*!
      Used to start a task in background.
     */

    void run(Task *task);

    /*!
      Release background thread and wait for its completion.

      Will freeze the calling (foreground) thread, so testing purposes only!
     */

    void releaseAndWait();

public slots:
    /*!
      Used by the BackgroundThread to emit when
      asynchronous calculation has been finished.
      */
    void onTaskDone(QuillImage& image, Task* task);

private:
    bool                    m_isRunning;
    Task                    *m_task;
    Quill::ThreadingMode    threadingMode;  
    QEventLoop              *eventLoop;
    BackgroundThread        *m_BackgroundThread;
};

#endif // __QUILL_THREAD_MANAGER_H_
