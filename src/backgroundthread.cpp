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

#include "backgroundthread.h"
#include "task.h"
#include <QMetaType>
#include <QuillImageFilter>

BackgroundThread::BackgroundThread(QObject *parent) :
    QThread(parent),
    m_IsStopped(false)
{
    // Registering QuillImage and Task type in order to use them in the signal-slot
    qRegisterMetaType<QuillImage>("QuillImage&");
    qRegisterMetaType<Task>("Task");
}

BackgroundThread::~BackgroundThread()
{
    m_IsStopped = true;
}

void BackgroundThread::stopBackgroundThread()
{
    m_IsStopped = true;
    m_WaitForTask.wakeAll();
    // Wait for the thread to finish its run() method.
    wait();
}

void BackgroundThread::run()
{
    // Thread will be suspended when there is no task.
    while( !m_IsStopped )
    {
        // Serialized access to the task queue list
        m_TaskMutex.lock();
        if( !m_TaskQueue.isEmpty() )
        {
            Task* task = m_TaskQueue.dequeue();
            m_TaskMutex.unlock();
            // Task is available, emit the signal which completes processFinishedTask
            QuillImage image = task->filter()->apply(task->inputImage());
            emit taskDone(image,task);
        }
        else
        {
            // Wait when there is no task
            m_WaitForTask.wait(&m_TaskMutex);
            m_TaskMutex.unlock();
            // Task is available so process it. Continue...
        }
    }
}

// Enqueue the tasks as fast as possible
void BackgroundThread::processTask(Task* task)
{
    // Serilizing access to task queue
    m_TaskMutex.lock();
    m_TaskQueue.enqueue(task);
    m_TaskMutex.unlock();
    // Wake the thread to process this task
    m_WaitForTask.wakeOne();
}
