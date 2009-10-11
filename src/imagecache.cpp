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

#include <QuillImage>
#include <QDebug>
#include "imagecache.h"

const ImageCache::KeyListPosition ImageCache::firstPosition = 0;
const ImageCache::KeyListPosition ImageCache::secondPosition = 1;
const ImageCache::KeyListPosition ImageCache::emptyPosition = -1;

ImageCache::ImageCache(int maxCost)
{
    keyList << emptyPosition << emptyPosition;
    cacheProtected.setMaxCost(1000000000);
    cache.setMaxCost(maxCost);
}

ImageCache::~ImageCache()
{
    // The destructors take care of emptying the caches - no need to
    // do it manually.
}

bool ImageCache::insertImage(int key, const QuillImage &image, ImageCache::ProtectionStatus status)
{
    QuillImage *imagePointer = new QuillImage(image);

    bool result=false;
    int cost = imagePointer->numBytes();
    //case 1: we just insert image to cache
    if(status == ImageCache::NotProtected)
        result = cache.insert(key,imagePointer,cost);

    //case 2: we need to take the first one from protected
    else if (status == ImageCache::ProtectFirst){
        if ((keyList.first() != emptyPosition) &&
            (keyList.last() != keyList.first()))
            deleteFromProtected(firstPosition);

        result = cacheProtected.insert(key, imagePointer, cost);

        //we update the keys in Qlist
        if (result)
            setKey(firstPosition, key);
    }
    //case 3: we need to take the last one from not delete
    else if (status == ImageCache::ProtectLast){
        if ((keyList.last() != emptyPosition) &&
            (keyList.last() != keyList.first()))
            deleteFromProtected(secondPosition);

        result = cacheProtected.insert(key, imagePointer, cost);

        if(result)
            setKey(secondPosition, key);
    }
    return result;
}

bool ImageCache::changeProtectionStatus(int key, ImageCache::ProtectionStatus status)
{
    bool result=false;
    if (cache.contains(key)){
	//we need to add cost
        if (status == ImageCache::ProtectFirst){
            result = internalChangeProtection(firstPosition,key);
            return result;
        }
        else {
            result = internalChangeProtection(secondPosition,key);
            return result;
        }
    }
    //change protection ->not protection or still protection (both last and first)
    else if (cacheProtected.contains(key)){
        KeyListPosition index = keyList.indexOf(key);

        //last and first
        if(status == ImageCache::ProtectFirst){
            //there is one image in first place
            if (keyList.last() == key){
                if (keyList.first() != key && keyList.first() != emptyPosition)
                    result = deleteFromProtected(firstPosition);

                setKey(firstPosition, key);
            }
            return result;
        }
        else if (status == ImageCache::ProtectLast){
            if (keyList.first() == key){
                if (keyList.last() != key && keyList.last() != emptyPosition)
                    result = deleteFromProtected(secondPosition);

                setKey(secondPosition, key);
            }
            return result;
        }
        else {
            result = deleteFromProtected(index);
            setKey(index, emptyPosition);
            return result;
        }
    }
    return result;
}


int  ImageCache::countCache() const
{
    return cache.count();
}

int ImageCache::countCacheProtected() const
{
    return cacheProtected.count();
}

bool ImageCache::cacheCheck(int key) const
{
    return cache.contains(key);
}

bool ImageCache::cacheProtectedCheck(int key) const
{
    return cacheProtected.contains(key);
}

QuillImage ImageCache::image(int key) const
{
    //check the cache
    if(cacheProtected.contains(key)){
        QuillImage *image = cacheProtected.object(key);
        return *image;
    }
    else if (cache.contains(key)){
        QuillImage *image = cache.object(key);
        return *image;
    }
    else
        return QuillImage();
}

int ImageCache::cacheTotalCost() const
{
    return cache.totalCost();
}

QList<int> ImageCache::checkKeys() const
{
    return cache.keys();
}

void ImageCache::setMaxCost(int maxCost)
{
    cache.setMaxCost(maxCost);
}

bool ImageCache::internalChangeProtection(int position, int key)
{
    bool result = false;
    QuillImage *image1 = cacheProtected.take(keyList.at(position));
    int size1 = image1->numBytes();
    result = cache.insert(keyList.at(position), image1, size1);
    QuillImage *image2 = cache.take(key);
    int size2 = image2->numBytes();
    result = cacheProtected.insert(key, image2, size2);
    cache.remove(key);
    //the images are not in order in cache
    result = cacheProtected.remove(keyList.at(position));
    keyList.removeAt(position);
    keyList.insert(position, key);
    return result;
}

bool ImageCache::deleteFromProtected(KeyListPosition position)
{
    bool result = false;
    QuillImage *image = cacheProtected.take(keyList.at(position));
    int size = image->numBytes();
    int key = keyList.at(position);
    result = cacheProtected.remove(key);
    result = cache.insert(key, image, size);
    return result;
}


void ImageCache::setKey(KeyListPosition position, int key)
{
     keyList.removeAt(position);
     keyList.insert(position,key);
}
