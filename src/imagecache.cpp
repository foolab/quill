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

#include <QuillImage>
#include "imagecache.h"

class CacheImage
{
public:
    QuillImage image;
    const File *file;
    int key;
};

ImageCache::ImageCache(int maxCost)
{
    m_cache.setMaxCost(maxCost);
    m_cacheProtected.setMaxCost(1);
}

ImageCache::~ImageCache()
{
    // The destructors take care of emptying the caches - no need to
    // do it manually.
}

bool ImageCache::insert(const File *file, int commandId,
                        const QuillImage &image, ProtectionStatus status)
{
    bool result = false;

    if (image.isNull())
        return result;

    CacheImage *cacheImage = new CacheImage;
    cacheImage->image = image;
    cacheImage->file = file;
    cacheImage->key = commandId;

    // Insert to not protected
    if (status == NotProtected)
        result = m_cache.insert(commandId, cacheImage);

    // Insert to protected
    else {
        CacheImage *oldImage = m_cacheProtected.take(file);

        // Move old one from protected to not protected
        if (oldImage) {
            if (oldImage->key != commandId)
                m_cache.insert(oldImage->key, oldImage);
            else
                delete oldImage;
        }

        m_cacheProtected.insert(file, cacheImage, 0);

        result = true;
    }

    return result;
}

bool ImageCache::protect(const File *file, int commandId)
{
    CacheImage *image = m_cache.take(commandId);

    if (image) {
        CacheImage *oldImage = m_cacheProtected.take(file);

        // Move old one from protected to not protected
        if (oldImage) {
            if (oldImage->key != commandId)
                m_cache.insert(oldImage->key, oldImage);
            else
                delete oldImage;
        }

        m_cacheProtected.insert(file, image, 0);

        return true;
    } else
        return false;
}

bool ImageCache::remove(const File *file, int commandId)
{
    CacheImage *cacheImage = m_cache.take(commandId);
    if (cacheImage) {
        delete cacheImage;
        return true;
    } else {
        cacheImage = m_cacheProtected.object(file);
        if (cacheImage && (cacheImage->key == commandId))
            return m_cacheProtected.remove(file);
    }
    return false;
}

bool ImageCache::purge(const File *file)
{
    return m_cacheProtected.remove(file);
}

bool ImageCache::hasImage(const File *file, int key) const
{
    if (m_cache.contains(key)) {
        CacheImage *cacheImage = m_cache.object(key);
        return !cacheImage->image.isNull();
    } else if (m_cacheProtected.contains(file)) {
        CacheImage *cacheImage = m_cacheProtected.object(file);
        if (cacheImage->key == key)
            return !cacheImage->image.isNull();
    }
    return false;
}

QuillImage ImageCache::image(const File *file, int key) const
{
    if (m_cache.contains(key)) {
        CacheImage *cacheImage = m_cache.object(key);
        return cacheImage->image;
    } else if (m_cacheProtected.contains(file)) {
        CacheImage *cacheImage = m_cacheProtected.object(file);
        if (cacheImage->key == key)
            return cacheImage->image;
    }

    return QuillImage();
}

int ImageCache::protectedId(const File *file) const
{
    if (m_cacheProtected.contains(file)) {
        CacheImage *image = m_cacheProtected.object(file);
        return image->key;
    } else
        return -1;
}

void ImageCache::setMaxSize(int maxSize)
{
    m_cache.setMaxCost(maxSize);
}

int ImageCache::maxSize() const
{
    return m_cache.maxCost();
}
