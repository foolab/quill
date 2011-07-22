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
  \class SaveMap

  \brief Contains save buffer handling necessary for serial access
  saving.

Each QuillFile in progress of saving has a SaveMap, which has a save
buffer. Individual tiles are inserted into the save buffer using a
special QuillImageFilter called Overlay. When the save buffer is full,
its contents are moved to the image writer for further processing.
 */

#ifndef __QUILL_SAVE_MAP_H_
#define __QUILL_SAVE_MAP_H_

#include <QSize>
#include <QRect>
#include <QList>
#include <QuillImage>

class QuillImage;
class TileMap;

class SaveMap
{
public:
    /*!
      Creates a save map.
    */

    SaveMap(const QSize &fullImageSize, int bufferHeight, TileMap *tileMap);

    ~SaveMap();

    /*!
      If we already have tiles in the tilemap which we can process
     */

    int processNext(TileMap *tileMap);

    /*!
      Which tile is required next
     */

    int prioritize();

    /*!
      Creates an "overlay" filter for processing the image into a buffer.
      Execute this filter on the background thread to receive the processed
      buffer.
    */

    QuillImageFilter *addToBuffer(int index);

    /*!
      Returns the current buffer.
     */

    QuillImage buffer() const;

    /*!
      Replaces the current buffer. This should be used when the background
      thread has finished processing the buffer.
    */

    void setBuffer(const QuillImage &buffer);

    /*!
      Destroys the current buffer and moves internal state to the next one.
    */

    void nextBuffer();

    /*!
      If the buffer is complete (having all its tiles set.)
    */

    bool isBufferComplete() const;

    /*!
      If the whole save operation has been completed.
    */

    bool isSaveComplete() const;

    /*!
      Count buffers.
     */

    int bufferCount() const;

private:
    QRect bufferArea(int bufferId) const;

    QSize m_fullImageSize;
    int m_bufferHeight;
    int m_bufferId;
    QuillImage m_buffer;
    QList<QList<int> > m_tileRows;
};

#endif // __QUILL_SAVE_MAP_H_
