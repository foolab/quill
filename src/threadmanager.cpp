/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Alexander Bokovoy <alexander.bokovoy@nokia.com>
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

#include <QtConcurrentRun>
#include <QSemaphore>
#include <QEventLoop>

#include <QuillImageFilter>

#include "quillerror.h"
#include "core.h"
#include "task.h"
#include "logger.h"
#include "threadmanager.h"

ThreadManager::ThreadManager(Quill::ThreadingMode mode) :
    m_isRunning(false), m_task(0), resultImage(0),
    threadingMode(mode),
    semaphore(0), eventLoop(0)
{
    if (mode == Quill::ThreadingTest)
    {
        semaphore = new QSemaphore();
        eventLoop = new QEventLoop();
    }

    connect(&watcher,
            SIGNAL(finished()),
            SLOT(taskFinished()));
}

ThreadManager::~ThreadManager()
{
    // If in test mode, make sure that background thread is not
    // left hanging on a destroyed semaphore
    if (threadingMode == Quill::ThreadingTest) {
        semaphore->release();
        if (isRunning())
            eventLoop->exec();
    }
    delete semaphore;
    delete eventLoop;
    delete resultImage;
}

bool ThreadManager::isRunning() const
{
    return m_isRunning;
}

QuillImage applyFilter(QuillImageFilter *filter, QuillImage image,
                       QSemaphore *semaphore)
{
    if (semaphore != 0)
        semaphore->acquire();
    return filter->apply(image);
}

void ThreadManager::run(Task *task)
{
    Logger::log(Logger::Module_ThreadManager, "Applying filter " + task->filter()->name());
    m_isRunning = true;
    m_task = task;

    resultImage = new QFuture<QuillImage>;
    *resultImage =
        QtConcurrent::run(applyFilter, task->filter(), task->inputImage(),
                          semaphore);
    watcher.setFuture(*resultImage);

    // To save memory, clear the input image here since it is not needed.
    task->setInputImage(QuillImage());
}

void ThreadManager::taskFinished()
{
    Logger::log(Logger::Module_ThreadManager, "Finished applying " + m_task->filter()->name());
    QuillImage image = resultImage->result();
    delete resultImage;
    resultImage = 0;
    m_isRunning = false;

    Core::instance()->processFinishedTask(m_task, image);

    if (threadingMode == Quill::ThreadingTest)
        eventLoop->exit();
}

bool ThreadManager::allowDelete(QuillImageFilter *filter) const
{
    return (!m_isRunning || !m_task || (filter != m_task->filter()));
}

void ThreadManager::releaseAndWait()
{
    if (threadingMode == Quill::ThreadingTest)
    {
        semaphore->release();
        if (isRunning())
            eventLoop->exec();
    }
}
