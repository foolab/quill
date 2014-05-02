#ifndef AV_THUMBNAILER_H
#define AV_THUMBNAILER_H

#include <QObject>
#include <QStringList>

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
  bool m_taskInProgress;
};

#endif
