/****************************************************************************
**
** Copyright (C) 2009-11 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Pekka Marjola <pekka.marjola@nokia.com>
**
** This file is part of the Quill package.
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

/*!
  \class Scheduler

  \brief Responsible for selecting the next task for the worker thread
  on the background.

Contains no explicit thread operations on its own, threading is done
by the QFuture and QFutureWatcher classes from Qt 4.5, which use
QThreadPool on the background.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QTemporaryFile>
#include "quill.h"

class QuillImage;
class File;
class Core;
class QuillUndoStack;
class QuillUndoCommand;
class Task;

class Scheduler : public QObject
{
Q_OBJECT

public:
    Scheduler();

    ~Scheduler();

    /*!
      If the thread manager is currently running a task in the background.
     */

    bool isRunning() const;

    /*!
      Processes a finished task with all consequences.
     */

    void processFinishedTask(Task *task, QuillImage image);

    /*!
      Used to get a new task to be given to ThreadManager.
      If no task is available, returns 0.
     */

    Task *newTask();

    /*!
      @return if the thread manager allows the given filter to be
      deleted (i.e. that it is not currently running it.
     */

    bool allowDelete(QuillImageFilter *filter) const;

    /*!
      Release background thread and wait for its completion.

      Will freeze the calling (foreground) thread, so testing purposes only!
     */

    void releaseAndWait();

private:
    /*!
      Used to indicate that there may be a normal task (loading or
      running an image filter) in this stack, waiting for the
      background thread.
     */

    Task *newNormalTask(File *file, int level);

    /*
      Go through all files above certain priority
      trying to find a normal task
     */

    Task *newNormalTask(const QList<File*> &fileList, int minPriority);

    /*!
      Used by core to indicate that there may be a save task waiting
      for the background thread.
     */

    Task *newSaveTask(File *file);

    /*!
      Used by core to indicate that there may be a special
      improvement task (better quality preview image to be created
      based on the full image) waiting for the background thread.
    */

    Task *newPreviewImprovementTask(File *file);

    /*
      Go through all files above certain priority
      trying to find a thumbnail load task
     */

    Task *newThumbnailLoadTask(const QList<File*> &fileList, int minPriority);

    /*
      Suggest to load a pre-generated thumbnail from a file.
     */

    Task *newThumbnailLoadTask(File *file, int level) const;

    /*!
      Suggest to save a thumbnail
     */

    Task *newThumbnailSaveTask(File *file, int level);

    /*
      Helper function for newNormalTask().
     */

    QuillUndoCommand *getTask(QuillUndoStack *stack, int level) const;

    /*!
      Helper function for suggestNewTask(), used for tiling.
      Can start calculations on its own.
    */

    Task *newTilingTask(File *file);

    /*!
      Helper function for suggestSaveTask(), used for tiling.
      Can start calculations on its own.
    */

    Task *newTilingSaveTask(File *file);

    /*!
      Helper function for suggestTilingSaveTask(), used for pushing
      tiles into the save buffer. Can start calculations on its own.
    */

    Task *newTilingOverlayTask(File *file);

    /*!
      Calculates full size, either based on the real full size or
      the best preview given by setImage()
     */

    static QSize fullSizeForAspectRatio(const File *file);
};

#endif // SCHEDULER_H
