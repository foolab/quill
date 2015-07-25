#ifndef AV_THUMBNAILER_H
#define AV_THUMBNAILER_H

#include <QObject>
#include <QStringList>
#include <thread>
#include <mutex>

class AVThumbnailer : public QObject {
  Q_OBJECT

public:
  AVThumbnailer();
  ~AVThumbnailer();

  bool supports(const QString mimeType);

  bool isRunning();

  void newThumbnailerTask(const QString &filePath,
			  const QString &mimeType,
			  const QString &flavor);

signals:
  void thumbnailGenerated(const QString filePath, const QString flavor);
  void thumbnailError(const QString filePath, uint errorCode,
		      const QString message);

private:
  static void createThumbnail(AVThumbnailer *that, QString flavor, QString inPath, QString outPath,
			      int width, bool crop);
  std::mutex m_mutex;
  std::thread m_thread;
  bool m_taskInProgress;
};

#endif
