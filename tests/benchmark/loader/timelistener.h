#include <QObject>
#include <QTime>

class TimeListener : public QObject
{
Q_OBJECT
public:
    TimeListener();
public slots:
    void timer();
private:
    QTime time;
};
