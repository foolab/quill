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
  \class ImageCache

  \brief A cache used to store preview and full images.

A different image cache is created by LibQuill for each display level.

ImageCache has two internal caches:

The "protected" cache holds the images related to the current state of
each respective file (or if the current state has not been calculated
yet, the state which is the most relevant for the calculation of the
current one). This cache does not have a size limit, but the number of
open files can be limited by QuillFile::setFileLimit().

The normal, or "not protected", cache is used to store non-current
edit history images which provide fast image recovery in the case of
undo. This part of the cache has an upper limit, and cache items
can be expired at any time.

ImageCache transparently handles moving items between these two
caches, caused by insert() and protect().

Due to different cache policies, ImageCache is not used to store
tiles - instead, TileCache is used for that.
 */

#ifndef QUILL_UNDO_CACHE_H
#define QUILL_UNDO_CACHE_H

#include <QObject>
#include <QCache>
#include <QMap>
#include <QList>

class QuillImage;
class QString;
class CacheImage;
class File;

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
      @param file the pointer to a File object
      @param commandId the unique id of the image
      @param image the image to be inserted
      @param ProtectionStatus the status of protection for this image
     */
    bool insert(const File *file, int commandId,
                const QuillImage &image,
                ProtectionStatus status = NotProtected);

    /*!
      Returns true if the image stored exists in the cache, false otherwise.
     */
    bool hasImage(const File *file, int commandId) const;

    /*!
      Returns the image stored in the cache.
     */
    QuillImage image(const File *file, int commandId) const;

    /*!
      Protect the image. This removes possible protection from any other
      command in the same file.

      Note that trying to change the status to "not protected" will
      have no effect.

      @param commandId the key of the image
     */
    bool protect(const File *file, int commandId);

    /*!
      Returns the command id of the image which is currently protected
      for the file.
     */

    int protectedId(const File *file) const;

    /*!
      Delete the image from the cache.

      @param commandId pointer to the command.

      @param file pointer to the file object, used for comparison
      purposes only.
     */

    bool remove(const File *file, int commandId);

    /*!
      Purge from the cache all images related to a given file.

      @param file pointer to the file object, used for comparison
      purposes only.
     */
    bool purge(const File *file);

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
    QCache<const File*, CacheImage> m_cacheProtected;
};



#endif //QUILL_UNDO_CACHE_H
