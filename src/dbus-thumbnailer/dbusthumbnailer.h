#ifndef DBUS_THUMBNAILER_H
#define DBUS_THUMBNAILER_H

#include <QObject>
#include <QStringList>

class DBusThumbnailer : public QObject {
Q_OBJECT

 public:
    DBusThumbnailer();
    ~DBusThumbnailer();

    bool supports(const QString mimeType);

    bool isRunning();

    void newThumbnailerTask(const QString &fileName,
                            const QString &mimeType,
                            const QString &flavor);

 signals:
    void thumbnailGenerated(const QString fileName);
    void thumbnailError(const QString fileName, int errorCode,
                        const QString message);

 private slots:
    void finishedHandler(uint handle);

    void errorHandler(uint handle, const QStringList failedUris,
                      int errorCode, const QString message);

 private:
    bool m_taskInProgress;
    QString m_taskFileName;
};

#endif
