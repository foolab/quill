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
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

bool AVThumbnailer::supports(const QString mimeType)
{
    return (!mimeType.isEmpty() && !mimeType.startsWith("image"));
}

bool AVThumbnailer::isRunning()
{
    bool running;
    m_mutex.lock();
    running = m_taskInProgress;
    m_mutex.unlock();

    return running;
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
    QSize min = Core::instance()->minimumPreviewSize(level);
    bool crop = min.isValid();
    QString path = File::filePathHash(filePath);
    path.append(Strings::dot + Core::instance()->thumbnailExtension());
    path.prepend(Core::instance()->thumbnailPath(level) +
                            QDir::separator());

    try {
      m_taskInProgress = true;
      if (m_thread.joinable()) {
	m_thread.join();
      }

      m_thread = std::thread(createThumbnail, this, flavor, filePath, path,
			     size.width(), crop);

    } catch (...) {
      m_taskInProgress = false;
      QMetaObject::invokeMethod(this, "thumbnailError", Qt::QueuedConnection,
				Q_ARG(QString, filePath), Q_ARG(uint, 0),
				Q_ARG(QString, QString()));
    }
}

void AVThumbnailer::createThumbnail(AVThumbnailer *that, QString flavor,
				    QString inPath, QString outPath,
				    int width, bool crop) {

  try {
    ffmpegthumbnailer::VideoThumbnailer
      thumbnailer(width, false, !crop, 8, false);
    thumbnailer.setSeekPercentage(10);
    thumbnailer.generateThumbnail(inPath.toStdString(), Jpeg, outPath.toStdString());
    that->m_mutex.lock();
    that->m_taskInProgress = false;
    that->m_mutex.unlock();
    QMetaObject::invokeMethod(that, "thumbnailGenerated", Qt::QueuedConnection,
			      Q_ARG(QString, inPath), Q_ARG(QString, flavor));
  } catch (std::exception& e) {
    qDebug() << e.what();
    that->m_mutex.lock();
    that->m_taskInProgress = false;
    that->m_mutex.unlock();
    QMetaObject::invokeMethod(that, "thumbnailError", Qt::QueuedConnection,
			      Q_ARG(QString, inPath), Q_ARG(uint, 0),
			      Q_ARG(QString, QString()));
  } catch (...) {
    that->m_mutex.lock();
    that->m_taskInProgress = false;
    that->m_mutex.unlock();
    QMetaObject::invokeMethod(that, "thumbnailError", Qt::QueuedConnection,
			      Q_ARG(QString, inPath), Q_ARG(uint, 0),
			      Q_ARG(QString, QString()));
  }
}
