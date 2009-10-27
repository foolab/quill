/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Alexander Bokovoy <alexander.bokovoy@nokia.com>
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

#ifndef QUILL_UNDO_CACHE_H
#define QUILL_UNDO_CACHE_H

#include <QObject>
#include <QCache>
#include <QMap>
#include <QList>

class QuillImage;
class QString;
class CacheImage;

class ImageCache: public QObject
{
Q_OBJECT

    friend class ut_imagecache;

public:
    /*!
      Enumeration values of protection status.
     */
    enum ProtectionStatus {
        NotProtected =0,
        Protected
    };

    /*!
      @param maxCost the max cost for cache
      @param maxNotDeleteCost the max cost for not deletable pictures
     */

    ImageCache(int maxCost);
    ~ImageCache();
    /*!
      Insert an image to the cache or the protected cache.
      @param file the pointer to a QuillFile object
      @param commandId the unique id of the image
      @param image the image to be inserted
      @param ProtectionStatus the status of protection for this image
     */
    bool insert(void *file, int commandId,
                const QuillImage &image,
                ProtectionStatus status = NotProtected);

    /*!
      Returns the image stored in the cache.
     */
    QuillImage image(void *file, int commandId) const;

    /*!
      Protect the image. This removes possible protection from any other
      command in the same file.

      Note that trying to change the status to "not protected" will
      have no effect.

      @param commandId the key of the image
     */
    bool protect(void *file, int commandId);

    /*!
      Returns the command id of the image which is currently protected
      for the file.
     */

    int protectedId(void *file) const;

    /*!
      Delete the image from the cache.

      @param commandId pointer to the command.

      @param file pointer to the file object, used for comparison
      purposes only.
     */

    bool remove(void *file, int commandId);

    /*!
      Purge from the cache all images related to a given file.

      @param file pointer to the file object, used for comparison
      purposes only.
     */
    bool purge(void *file);

    /*!
      Change the max size of the cache.

      Images with a protected status do not count towards this max size.
    */
    void setMaxSize(int maxSize);

    /*!
      The max size of the cache.
    */
    int maxSize() const;

private:

    QCache<int, CacheImage> m_cache;
    QCache<void*, CacheImage> m_cacheProtected;
};



#endif //QUILL_UNDO_CACHE_H
