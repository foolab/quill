#ifndef DBUS_THUMBNAILER_H
#define DBUS_THUMBNAILER_H

#include <QObject>
#include <QStringList>
class ThumbnailerGenericProxy;

class DBusThumbnailer : public QObject {
Q_OBJECT

friend class ut_dbusthumbnailer;
 public:
    DBusThumbnailer();
    ~DBusThumbnailer();

    bool supports(const QString mimeType);

    bool isRunning();

    void newThumbnailerTask(const QString &fileName,
                            const QString &mimeType,
                            const QString &flavor);

 signals:
    void thumbnailGenerated(const QString fileName, const QString flavor);
    void thumbnailError(const QString fileName, uint errorCode,
                        const QString message);

 private slots:
    void finishedHandler(uint handle);

    void errorHandler(uint handle, const QStringList failedUris,
                      int errorCode, const QString message);

 private:
    void connectDBus();

 private:
    static QString tumblerService, tumblerCache;

    bool m_taskInProgress;
    QString m_taskFileName;
    QString m_flavor;
    ThumbnailerGenericProxy *m_tumbler;
};

#endif
