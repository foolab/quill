#include "avthumbnailer.h"
#include "logger.h"
#include "file.h"
#include "strings.h"
#include "core.h"
#include <libffmpegthumbnailer/videothumbnailer.h>
#include <QDir>
#include <QDebug>

AVThumbnailer::AVThumbnailer() :
  m_taskInProgress(false)
{
}

AVThumbnailer::~AVThumbnailer()
{
}

bool AVThumbnailer::supports(const QString mimeType)
{
    return (!mimeType.isEmpty() && !mimeType.startsWith("image"));
}

bool AVThumbnailer::isRunning()
{
    return m_taskInProgress;
}

void AVThumbnailer::newThumbnailerTask(const QString &filePath,
                                         const QString &mimeType,
                                         const QString &flavor)
{
    Q_UNUSED(mimeType);

    QUILL_LOG(Logger::Module_DBusThumbnailer, QString(Q_FUNC_INFO));

    if (isRunning())
        return;

    m_taskInProgress = true;
    int level = Core::instance()->levelFromFlavor(flavor);
    QSize size = Core::instance()->previewSize(level);
    QString path = File::filePathHash(filePath);
    path.append(Strings::dot + Core::instance()->thumbnailExtension());
    path.prepend(Core::instance()->thumbnailPath(level) +
                            QDir::separator());

    qDebug() << filePath << flavor << size;

    try {
      ffmpegthumbnailer::VideoThumbnailer
	thumbnailer(size.width(), false, size.width() != size.height(), 8, false);
      thumbnailer.setSeekPercentage(10);
      thumbnailer.generateThumbnail(filePath.toStdString(), Jpeg, path.toStdString());
      m_taskInProgress = false;
      QMetaObject::invokeMethod(this, "thumbnailGenerated", Qt::QueuedConnection,
				Q_ARG(QString, filePath), Q_ARG(QString, flavor));
    } catch (std::exception& e) {
      qDebug() << e.what();
      m_taskInProgress = false;
      QMetaObject::invokeMethod(this, "thumbnailError", Qt::QueuedConnection,
				Q_ARG(QString, filePath), Q_ARG(uint, 0),
				Q_ARG(QString, QString()));
    } catch (...) {
      m_taskInProgress = false;
      QMetaObject::invokeMethod(this, "thumbnailError", Qt::QueuedConnection,
				Q_ARG(QString, filePath), Q_ARG(uint, 0),
				Q_ARG(QString, QString()));
    }
}
