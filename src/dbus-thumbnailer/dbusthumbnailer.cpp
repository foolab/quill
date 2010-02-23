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
    connect( DBusServices::instance(),
             SIGNAL(ReadyHandler(uint, const QStringList&)),
             SLOT(ReadyHandler(uint, const QStringList&)));
    connect( DBusServices::instance(),
             SIGNAL(StartedHandler(uint)),
             SLOT(StartedHandler(uint)));
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

    qDebug() << uri.toString() << mimeType << flavor;

    QStringList mimes;
    mimes.append(mimeType);

    QDBusPendingReply<uint> reply;
    reply = DBusServices::tumbler()->Queue(uris, mimes, flavor, "default", 0);

    qDebug() << "Putting reply";
}

void DBusThumbnailer::finishedHandler(uint handle)
{
    qDebug() << "Finishedhandler" << handle;

    emit thumbnailGenerated(m_taskFileName);

    m_taskInProgress = false;
}

void DBusThumbnailer::errorHandler(uint handle, const QStringList failedUris,
                                   int errorCode, const QString message)
{
    qDebug() << "errorHandler" << handle;
    emit thumbnailError(failedUris.first(), errorCode, message);
    m_taskInProgress = false;
}

void DBusThumbnailer::StartedHandler(uint handle)
{
    Q_UNUSED(handle);
    qDebug()<<"+++VideoThumbnailer::StartedHandler:it catches the started signal";
}

void DBusThumbnailer::ReadyHandler(uint handle, const QStringList &uris)
{
    Q_UNUSED(handle);
    Q_UNUSED(uris);
    qDebug()<<"+++VideoThumbnailer::StartedHandler:it catches the ready signal";
}
