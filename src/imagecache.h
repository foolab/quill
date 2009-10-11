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
class ImageKey;

class ImageCache: public QObject
{
Q_OBJECT

    friend class ut_imagecache;

public:
    /*!
      Enumeration values of protection status.
     */
    enum ProtectionStatus
    {NotProtected =0,
     ProtectFirst,
     ProtectLast
    };

    /*!
      @param maxCost the max cost for cache
      @param maxNotDeleteCost the max cost for not deletable pictures
     */

    ImageCache(int maxCost);
    ~ImageCache();
    /*!
      Insert an image to cache or not deletable memory.
      @param key the unique id of the image
      @param image the image to be inserted to cache
      @param ProtectionStatus the status of protection for this image
     */
    bool insertImage(int key, const QuillImage &image, ImageCache::ProtectionStatus=ImageCache::NotProtected);

    /*!
      Take the image stored in cache or not delete cache
      @return QuillImage* the pointer to QuillImage. The caller has the ownership
     */
    QuillImage image(int key) const;

    /*!
      Change the protection status of one protected image
      @param uniqueId the key of the image
      @param status the protection status to be changed

      Note that trying to change the status to "not protected" will
      have no effect.
     */
    bool changeProtectionStatus(int uniqueId,
                                ImageCache::ProtectionStatus status);

    /*!
      Change the max cost of the cache.

      Images with a protected status do not count towards this max cost.
    */
    void setMaxCost(int maxCost);

private:

    typedef int KeyListPosition;

    static const KeyListPosition firstPosition, secondPosition, emptyPosition;

    /*!
      Count the number of small pictures in cache
    */

    int countCache() const;
    /*!
      Count the number of not deletable pictures in cache
     */
    int countCacheProtected() const;

    /*!
      Check if one small picture exists in cache by its key
     */
    bool cacheCheck(int key) const;
    /*!
      Check if one not deletable picture exists in cache by its key
     */
    bool cacheProtectedCheck(int key) const;

    /*!
      Check the total cost in small picture cache
     */
    int cacheTotalCost() const;

    /*!
      Check keys stored in small picture cache
     */
    QList<int> checkKeys() const;

    /*!
      Change the protection status between not delete and cache
      @param position first or last protected
      @param key the key of image whose status will be changed
     */
    bool internalChangeProtection(KeyListPosition position, int key);
    /*!
      Delete one image from not delete memory because we will insert one more
     */
    bool deleteFromProtected(KeyListPosition position);

    /*!
      Change keys in QList to keep track of protected image
     */
    void setKey(KeyListPosition position, int key);

private:

    QCache<int, QuillImage> cache, cacheProtected;

    /*!
      Keep the key of images. There are 3 elements.
      The first two are the keys and last one is the position if
     */
    QList<int> keyList;

};



#endif //QUILL_UNDO_CACHE_H
