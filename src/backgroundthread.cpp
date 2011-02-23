#include "backgroundthread.h"
#include "task.h"
#include <QMetaType>
#include <QuillImageFilter>
#include <QSemaphore>

BackgroundThread::BackgroundThread(QObject *parent, QSemaphore* semaphore) :
    QThread(parent),
    m_IsStopped(false),
    m_Semaphore(semaphore)
{
    // Registering QuillImage and Task type in order to use them in the signal-slot
    qRegisterMetaType<QuillImage>("QuillImage&");
    qRegisterMetaType<Task>("Task*");
}

BackgroundThread::~BackgroundThread()
{
    m_IsStopped = true;
}

void BackgroundThread::stopBackgroundThread()
{
    m_IsStopped = true;
    m_WaitForTask.wakeAll();
    // Wait for all the thread finishes its run() method.
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
            if (m_Semaphore != 0)
            {
                m_Semaphore->acquire();
            }
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
