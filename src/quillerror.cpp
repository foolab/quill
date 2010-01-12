#include "quillerror.h"

class QuillErrorPrivate {
public:
    QuillError::ErrorCode errorCode;
    QuillError::ErrorSource errorSource;
    QString errorData;
};

QuillError::QuillError(ErrorCode errorCode) :
    priv(new QuillErrorPrivate)
{
    priv->errorCode = errorCode;
    priv->errorSource = NoErrorSource;
    priv->errorData = QString();
}

QuillError::QuillError(ErrorCode errorCode,
                       ErrorSource errorSource,
                       const QString &errorData) :
    priv(new QuillErrorPrivate)
{
    priv->errorCode = errorCode;
    priv->errorSource = errorSource;
    priv->errorData = errorData;
}

QuillError::QuillError(const QuillError &quillError) :
    priv(new QuillErrorPrivate)
{
    priv->errorCode = quillError.errorCode();
    priv->errorSource = quillError.errorSource();
    priv->errorData = quillError.errorData();
}

QuillError::~QuillError()
{
    delete priv;
}

QuillError::ErrorCode QuillError::errorCode() const
{
    return priv->errorCode;
}

void QuillError::setErrorCode(QuillError::ErrorCode errorCode)
{
    priv->errorCode = errorCode;
}

QuillError::ErrorSource QuillError::errorSource() const
{
    return priv->errorSource;
}

void QuillError::setErrorSource(QuillError::ErrorSource errorSource)
{
    priv->errorSource = errorSource;
}

QString QuillError::errorData() const
{
    return priv->errorData;
}

void QuillError::setErrorData(QString errorData)
{
    priv->errorData = errorData;
}

QuillError QuillError::operator=(const QuillError &quillError)
{
    priv->errorCode = quillError.errorCode();
    priv->errorSource = quillError.errorSource();
    priv->errorData = quillError.errorData();
    return *this;
}

// Translates Quill image filter errors to Quill errors

QuillError::ErrorCode QuillError::translateFilterError(QuillImageFilter::ImageFilterError error)
{
    switch(error) {
    case QuillImageFilter::NoError:
        return QuillError::NoError;
    case QuillImageFilter::FileNotFoundError:
        return QuillError::FileNotFoundError;
    case QuillImageFilter::UnsupportedFormatError:
        return QuillError::FileFormatUnsupportedError;
    case QuillImageFilter::InvalidDataError:
        return QuillError::FileCorruptError;
    default:
        return QuillError::UnspecifiedError;
    }
}
