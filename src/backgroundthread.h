#ifndef TASKPROCESSOR_H
#define TASKPROCESSOR_H

#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QMutex>

class Task;
class QuillImage;
class QSemaphore;

class BackgroundThread : public QThread
{
    Q_OBJECT
public:
    explicit BackgroundThread(QObject *parent = 0, QSemaphore* semaphore = 0);
    ~BackgroundThread();
    void processTask(Task* task);
    void stopBackgroundThread();

    // Virtual method from QThread
    void run();

signals:
    void taskDone(QuillImage& image, Task* task);

private:
    QQueue<Task*>   m_TaskQueue;
    bool            m_IsStopped;
    QWaitCondition  m_WaitForTask;
    QMutex          m_TaskMutex;
    QSemaphore*     m_Semaphore;
};

#endif // TASKPROCESSOR_H
