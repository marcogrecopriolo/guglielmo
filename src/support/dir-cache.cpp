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
	log(LOG_CACHE, LOG_CHATTY, "Cache dir does not exist %s", qPrintable(cacheDir));
	return;
    }
    log(LOG_CACHE, LOG_MIN, "loading %s", qPrintable(cacheDir));

    QStringList channelDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& serviceId : channelDirs) {
	QString channelPath = cacheDir + "/" + serviceId;
	QDir channelDir(channelPath);

	QStringList files = channelDir.entryList(QDir::Files);

	for (const QString& filename : files) {
	    QRegularExpression pattern("^(\\d+)x(\\d+)-([0-9a-fA-F]{4})\\.(\\w+)$");
	    QRegularExpressionMatch match = pattern.match(filename);

	    if (match.hasMatch()) {
		int width = match.captured(1).toInt();
		int height = match.captured(2).toInt();
		QString hash = match.captured(3).toLower();
		QString imageType = match.captured(4).toLower();

		QString key = hashKey(serviceId.toLower(), width, height, imageType);
		QString fullPath = channelPath + "/" + filename;

		cache[key] = fullPath;

		ImageInfo newInfo{fullPath, width, height, imageType, hash};
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
    QString key = hashKey(toServiceId(SId), width, height, imageType);
    return cache.contains(key);
}
    
QList<ImageInfo> ImageCache::getImagesForChannel(int32_t SId) {
    QString serviceId = toServiceId(SId);
    log(LOG_CACHE, LOG_CHATTY, "searching images for %s", qPrintable(serviceId));
    return channelIndex.values(serviceId);
}
    
void ImageCache::addImage(int32_t SId, int width, int height, 
		  const QString& imageType, const QByteArray& image) {
    QString serviceId = toServiceId(SId);
    QString key = hashKey(serviceId, width, height, imageType);
    QString imageHash = calculateHash(image);
    QString filename = QString("%1x%2-%3.%4")
	.arg(width).arg(height).arg(imageHash).arg(imageType.toLower());
    QString channelPath = QDir::home().absoluteFilePath(cacheDir + "/" + serviceId);
    QString fullPath = channelPath + "/" + filename;

    QString existingPath = cache.value(key, "");
    if (existingPath == fullPath) {
	log(LOG_CACHE, LOG_CHATTY, "skipping unchanged image %s", qPrintable(existingPath));
	return;
    } else if (existingPath != "") {
	cache.remove(key);
	removeFromChannelIndex(serviceId, width, height, imageType);
	QFile::remove(existingPath);
	log(LOG_CACHE, LOG_CHATTY, "replacing mage %s", qPrintable(existingPath));
    } else
	log(LOG_CACHE, LOG_MIN, "writing image %s", qPrintable(fullPath));
    QDir dir;
    dir.mkpath(channelPath);

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
	log(LOG_CACHE, LOG_MIN, "could not open %s for writing", qPrintable(fullPath));
	return;
    }
    file.write(image);
    file.close();

    cache[key] = fullPath;
    ImageInfo info{fullPath, width, height, imageType, imageHash};
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

QString ImageCache::hashKey(QString serviceId, int width, int height, const QString& imageType) {
    return QString("%1/%2x%3.%4")
	.arg(serviceId).arg(width).arg(height).arg(imageType.toLower());
}

QString ImageCache::calculateHash(const QByteArray& data) {
    uint hash = qHash(data);
    return QString("%1").arg(hash & 0xFFFF, 4, 16, QChar('0'));
}
