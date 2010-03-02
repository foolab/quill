#include "thumbnailer_generic.h"
#include "dbus-services.h"
#include "dbusthumbnailer.h"

DBusThumbnailer::DBusThumbnailer() : m_taskInProgress(false)
{
	connect( DBusServices::instance(),
             SIGNAL(FinishedHandler(uint)),
             SLOT(finishedHandler(uint)));
	connect( DBusServices::instance(),
             SIGNAL(ErrorHandler(uint,const QStringList,int,const QString&)),
             SLOT(errorHandler(uint,const QStringList,int,const QString)));
}

DBusThumbnailer::~DBusThumbnailer()
{
}

bool DBusThumbnailer::supports(const QString mimeType)
{
    return ((mimeType == "video/mp4") ||
            (mimeType == "image/jpeg"));
}

bool DBusThumbnailer::isRunning()
{
    return m_taskInProgress;
}

void DBusThumbnailer::newThumbnailerTask(const QString &fileName,
                                         const QString &mimeType,
                                         const QString &flavor)
{
    if (isRunning())
        return;

    m_taskInProgress = true;

    m_taskFileName = fileName;

    const QUrl uri =
        QUrl::fromLocalFile(QFileInfo(fileName).canonicalFilePath());
    QStringList uris;
    uris.append(uri.toString());

    QStringList mimes;
    mimes.append(mimeType);

    QDBusPendingReply<uint> reply;
    reply = DBusServices::tumbler()->Queue(uris, mimes, flavor, "default", 0);

}

void DBusThumbnailer::finishedHandler(uint handle)
{
    qDebug()<<"DBusThumbnailer::finishedHandler";
    Q_UNUSED(handle);
    emit thumbnailGenerated(m_taskFileName);
    qDebug()<<"DBusThumbnailer::finishedHandler():it emits the signal";
    m_taskInProgress = false;
}

void DBusThumbnailer::errorHandler(uint handle, const QStringList failedUris,
                                   int errorCode, const QString message)
{
    qDebug()<<"DBusThumbnailer::errorHandler";
    Q_UNUSED(handle);
    emit thumbnailError(failedUris.first(), errorCode, message);
    qDebug()<<"DBusThumbnailer::errorHandler():it emits the signal";
    m_taskInProgress = false;
}

