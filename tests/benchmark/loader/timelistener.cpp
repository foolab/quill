#include "timelistener.h"
#include <QDebug>

TimeListener::TimeListener() {
    time.start();
}

void TimeListener::timer() {
    qDebug() << "Time elapsed for tile:" << time.elapsed() << "ms";
    time.restart();
}
