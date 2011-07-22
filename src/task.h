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
  \class Task

  \brief Represents one task to be run on the background by ThreadManager.

  Tasks are created using the scheduling policy in
  Scheduler::newTask(). Since a task is run by the background thread,
  it does not contain any complex datatypes like QuillUndoCommands
  which might simultaneously be changed by the foreground thread.
*/

#include <QuillImage>

class QuillImageFilter;

class Task {
 public:

    Task();

    ~Task();

    /*!
      Gets the id of the task command (used to identify the command
      after the calculation, sharing the command would not be thread safe).
     */

    int commandId() const;

    /*!
      Sets the task command id.
     */
    void setCommandId(int commandId);

    /*!
      Gets the display level of the task.
     */

    int displayLevel() const;

    /*!
      Sets the display level of the task.
    */

    void setDisplayLevel(int displayLevel);

    /*!
      Gets the tile id of the task. If the display level is something
      else than the highest one, this has no effect.
     */

    int tileId() const;

    /*!
      Sets the tile id of the task.
     */

    void setTileId(int tileId);

    /*!
      Gets the input image of the task.
     */

    QuillImage inputImage() const;

    /*!
      Sets the input image of the task.
     */

    void setInputImage(const QuillImage &inputImage);

    /*!
      Gets the filter of the task.
     */

    QuillImageFilter *filter() const;

    /*!
      Sets the filter of the task. The filter stays the property of the caller,
      but it must not be destroyed as long as the task exists.
     */

    void setFilter(QuillImageFilter *filter);

 private:
    int m_commandId;
    int m_displayLevel;
    int m_tileId;
    QuillImage m_inputImage;
    QuillImageFilter *m_filter;
    QString m_fileName;
};
