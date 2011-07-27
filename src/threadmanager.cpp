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

#include <QEventLoop>
#include "quillerror.h"
#include "core.h"
#include "task.h"
#include "logger.h"
#include "threadmanager.h"
#include "backgroundthread.h"

ThreadManager::ThreadManager(Quill::ThreadingMode mode) :
    m_isRunning(false), m_task(0), threadingMode(mode),
    eventLoop(0)
{
    if (mode == Quill::ThreadingTest) {
        eventLoop = new QEventLoop();
    }

    // BackgroundThread is like a worker thread which processes Task in a same manner
    // as with QtConcurrent implementation.
    m_BackgroundThread = new BackgroundThread(this);
    // Here we can tune thread priority according to our needs.
    m_BackgroundThread->start(QThread::LowPriority);
    // As soon as BackgroundThread::run() method applies the required filter, it emits
    // signal to background thread.
    QObject::connect(m_BackgroundThread,SIGNAL(taskDone(QuillImage&,Task*)),this,SLOT(onTaskDone(QuillImage&,Task*)),Qt::UniqueConnection);
}

ThreadManager::~ThreadManager()
{    
    // Stoping the BackgroundThread.
    // the instance will be deleted after destruction of this class
    m_BackgroundThread->stopBackgroundThread();
    if (threadingMode == Quill::ThreadingTest) {
        delete eventLoop;
    }
}

bool ThreadManager::isRunning() const
{
    return m_isRunning;
}

void ThreadManager::run(Task *task)
{
    QUILL_LOG(Logger::Module_ThreadManager, "Applying filter " + task->filter()->name());
    m_isRunning = true;
    m_task = task;

    // For unit testing, task will be processed by calling ThreadManager::releaseAndWait()
    if (threadingMode != Quill::ThreadingTest) {
        // Enqueues the task in the queue to process by BackgroundThread thread.
        m_BackgroundThread->processTask(task);
    }
}

// BackgroundThread emits taskDone signal to this background thread
void ThreadManager::onTaskDone(QuillImage& image,Task* task)
{
    QUILL_LOG(Logger::Module_ThreadManager, "Finished applying " + m_task->filter()->name());
    m_isRunning = false;
    Core::instance()->processFinishedTask(task, image);

    if (threadingMode == Quill::ThreadingTest) {
        eventLoop->exit();
    }
}

bool ThreadManager::allowDelete(QuillImageFilter *filter) const
{
    return (!m_isRunning || !m_task || ( filter != m_task->filter()));
}

void ThreadManager::releaseAndWait()
{
    if (threadingMode == Quill::ThreadingTest)
    {
        if (m_isRunning) {
            m_BackgroundThread->processTask(m_task);
            eventLoop->exec();
        }
    }
}
