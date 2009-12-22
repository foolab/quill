#ifndef __QUILL_ERROR_H__
#define __QUILL_ERROR_H__
class QuillError
{
public:
    enum QuillFileError {
        NoError = 0,
        ReadError,
        WriteError,
        FatalError,
        ResourceError,
        OpenError,
        AbortError,
        TimeOutError,
        UnspecifiedError,
        RemoveError,
        RenameError,
        PositionError,
        ResizeError,
        PermissionsError,
        CopyError,
        MakePathError
    };
};

#endif //__QUILL_ERROR_H__
