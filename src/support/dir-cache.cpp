/*
 *    Copyright (C) 2021
 *    Marco Greco <marcogrecopriolo@gmail.com>
 *
 *    This file is part of the guglielmo FM DAB tuner software package.
 *
 *    guglielmo is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    guglielmo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with guglielmo; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dir-cache.h"
#include "logging.h"

ImageCache::ImageCache(const QString& relativeCacheDir) {
    cacheDir = QDir::home().absoluteFilePath(relativeCacheDir);
    loadCache();
}

ImageCache::~ImageCache() {
    cache.clear();
    channelIndex.clear();
}

void ImageCache::loadCache() {
    cache.clear();
    channelIndex.clear();

    QDir dir(cacheDir);
    if (!dir.exists()) {
	log(LOG_CACHE, LOG_CHATTY, "Cache dia does not exist %s", qPrintable(cacheDir));
	return;
    }
    log(LOG_CACHE, LOG_MIN, "loading %s", qPrintable(cacheDir));

    QStringList channelDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& serviceId : channelDirs) {
	QString channelPath = cacheDir + "/" + serviceId;
	QDir channelDir(channelPath);

	QStringList files = channelDir.entryList(QDir::Files);

	for (const QString& filename : files) {
	    QRegularExpression pattern("^(\\d+)x(\\d+)\\.(\\w+)$");
	    QRegularExpressionMatch match = pattern.match(filename);

	    if (match.hasMatch()) {
		int width = match.captured(1).toInt();
		int height = match.captured(2).toInt();
		QString imageType = match.captured(3);

		QString hashKey = QString("%1/%2x%3.%4")
		    .arg(serviceId).arg(width).arg(height).arg(imageType);

		QString fullPath = channelPath + "/" + filename;

		cache[hashKey] = fullPath;

		ImageInfo newInfo{fullPath, width, height, imageType};
		log(LOG_CACHE, LOG_CHATTY, "found %s %s %i %i", qPrintable(fullPath), qPrintable(serviceId), width, height);
		removeFromChannelIndex(serviceId, width, height, imageType);

		channelIndex.insert(serviceId, newInfo);
	    }
	}
    }

    log(LOG_CACHE, LOG_MIN, "Cached %i image paths", cache.size());
}
    
int ImageCache::imageCount() {
    return cache.size();
}

bool ImageCache::hasImage(int32_t SId, int width, int height, 
		  const QString& imageType) {
    QString hashKey = QString("%1/%2x%3.%4")
	.arg(toServiceId(SId)).arg(width).arg(height).arg(imageType);
    return cache.contains(hashKey);
}
    
QList<ImageInfo> ImageCache::getImagesForChannel(int32_t SId) {
    QString serviceId = toServiceId(SId);
    log(LOG_CACHE, LOG_CHATTY, "searching images for %s", qPrintable(serviceId));
    return channelIndex.values(serviceId);
}
    
void ImageCache::addImage(int32_t SId, int width, int height, 
		  const QString& imageType, const QByteArray& image) {
    QString serviceId = toServiceId(SId);
    QString channelPath = QDir::home().absoluteFilePath(cacheDir + "/" + serviceId);
    QDir dir;
    dir.mkpath(channelPath);

    QString filename = QString("%1x%2.%3").arg(width).arg(height).arg(imageType.toLower());
    QString fullPath = channelPath + "/" + filename;
    log(LOG_CACHE, LOG_MIN, "writing image SId %s", qPrintable(fullPath));

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
	log(LOG_CACHE, LOG_MIN, "could not open %s for writing", qPrintable(fullPath));
	return;
    }
    file.write(image);
    file.close();

    QString hashKey = QString("%1/%2x%3.%4")
	.arg(serviceId).arg(width).arg(height).arg(imageType);
    cache[hashKey] = fullPath;

    removeFromChannelIndex(serviceId, width, height, imageType);

    ImageInfo info{fullPath, width, height, imageType};
    channelIndex.insert(serviceId, info);
}

QPixmap ImageCache::loadImage(const QString& path) {
    QPixmap p;
    QString fullPath = QDir::home().absoluteFilePath(path);

    log(LOG_CACHE, LOG_MIN, "loading image %s", qPrintable(fullPath));
    p.load(fullPath);
    return p;
}

void ImageCache::removeFromChannelIndex(const QString& serviceId, int width, 
				int height, const QString& imageType) {
    QList<ImageInfo> entries = channelIndex.values(serviceId);

    for (const ImageInfo& entry : entries) {
	if (entry.width == width && 
	    entry.height == height && 
	    entry.imageType == imageType) {
	    channelIndex.remove(serviceId, entry);
	    break;
	}
    }
}
