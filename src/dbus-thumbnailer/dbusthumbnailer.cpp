#include "dbusthumbnailer.h"
#include "thumbnailer_generic.h"
#include "logger.h"
#include "file.h"

QLatin1String DBusThumbnailer::tumblerService("org.freedesktop.thumbnails.Thumbnailer1");
QLatin1String DBusThumbnailer::tumblerCache("/org/freedesktop/thumbnails/Thumbnailer1");

DBusThumbnailer::DBusThumbnailer() : m_taskInProgress(false),
                                     m_tumbler(0)
{
    connectDBus();
}

DBusThumbnailer::~DBusThumbnailer()
{
    delete m_tumbler;
}

void DBusThumbnailer::connectDBus()
{
    delete m_tumbler;
    m_tumbler = new ThumbnailerGenericProxy(tumblerService, tumblerCache,
                                            QDBusConnection::sessionBus());
    connect(m_tumbler,
            SIGNAL(Finished(uint)),
            SLOT(finishedHandler(uint)));
    connect(m_tumbler,
            SIGNAL(Error(uint,const QStringList,int,const QString)),
            SLOT(errorHandler(uint,const QStringList,int,const QString)));
}

bool DBusThumbnailer::supports(const QString mimeType)
{

    return (!mimeType.isEmpty() && !mimeType.startsWith("image"));
}

bool DBusThumbnailer::isRunning()
{
    return m_taskInProgress;
}

void DBusThumbnailer::newThumbnailerTask(const QString &filePath,
                                         const QString &mimeType,
                                         const QString &flavor)
{
    QUILL_LOG(Logger::Module_DBusThumbnailer, QString(Q_FUNC_INFO));

    if (isRunning())
        return;

    m_taskInProgress = true;
    m_taskFilePath = filePath;
    m_flavor = flavor;

    QStringList uris;
    uris.append(File::filePathToUri(filePath));

    QStringList mimes;
    mimes.append(mimeType);

    if ((!m_tumbler) || (!m_tumbler->isValid()))
        connectDBus();

    if (m_tumbler){
        m_tumbler->Queue(uris, mimes, flavor, "default", 0);
                         }
}

void DBusThumbnailer::finishedHandler(uint handle)
{
    Q_UNUSED(handle);
    emit thumbnailGenerated(m_taskFilePath, m_flavor);
    m_taskInProgress = false;
}

void DBusThumbnailer::errorHandler(uint handle, const QStringList failedUris,
                                   int errorCode, const QString message)
{
    Q_UNUSED(handle);
    Q_UNUSED(failedUris);
    emit thumbnailError(m_taskFilePath, errorCode, message);

}

