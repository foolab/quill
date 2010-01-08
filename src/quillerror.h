#ifndef __QUILL_ERROR_H__
#define __QUILL_ERROR_H__

#include <QMetaType>

class QString;
class QuillErrorPrivate;

class QuillError
{
 public:
    enum ErrorCode {
        UnspecifiedError = -1,
        NoError = 0,

        FileNotFoundError,
        FileOpenForReadError,
        FileReadError,
        FileFormatUnsupportedError,
        FileCorruptError,

        DirCreateError,
        FileOpenForWriteError,
        FileWriteError,

        FileRemoveError,

        GlobalFileLimitError,
        FilterGeneratorError
    };

    enum ErrorSource {
        UnspecifiedErrorSource = -1,
        NoErrorSource = 0,

        ImageFileErrorSource,
        ImageOriginalErrorSource,
        ThumbnailErrorSource,
        TemporaryFileErrorSource,
        EditHistoryErrorSource,
        CrashDumpErrorSource
    };

 public:
    QuillError(ErrorCode errorCode = NoError);
    QuillError(ErrorCode errorCode,
               ErrorSource errorSource,
               const QString &errorData);
    QuillError(const QuillError &quillError);

    ~QuillError();

    ErrorCode errorCode() const;
    void setErrorCode(ErrorCode errorCode);

    ErrorSource errorSource() const;
    void setErrorSource(ErrorSource errorSource);

    QString errorData() const;
    void setErrorData(QString errorData);

    QuillError operator=(const QuillError &quillError);

 private:
    QuillErrorPrivate *priv;
};

Q_DECLARE_METATYPE(QuillError)

#endif //__QUILL_ERROR_H__
