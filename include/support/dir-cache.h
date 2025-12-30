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

#ifndef __DIR_CACHE_H__
#define __DIR_CACHE_H__

#include <QHash>
#include <QMultiHash>
#include <QDir>
#include <QString>
#include <QList>
#include <QPixmap>
#include <QRegularExpression>

struct ImageInfo {
    QString path;
    int width;
    int height;
    QString imageType;

    bool operator==(const ImageInfo& other) const {
	return width == other.width && 
	       height == other.height && 
	       imageType == other.imageType;
    }
};
    
class ImageCache {
public:
    ImageCache(const QString& cacheDir);
    ~ImageCache();

    QString toServiceId(int32_t SId) { return QString::number(SId, 16).toLower(); }
    int imageCount();
    bool hasImage(int32_t SId, int width, int height, 
		  const QString& imageType = "png");
    QList<ImageInfo> getImagesForChannel(int32_t SId);
    void addImage(int32_t SId, int width, int height, 
		  const QString& imageType, const QByteArray& image);
    QPixmap loadImage(const QString& path);
    
private:
    QString cacheDir;
    QHash<QString, QString> cache;
    QMultiHash<QString, ImageInfo> channelIndex;
    
    void loadCache(void);
    void removeFromChannelIndex(const QString& serviceId, int width, 
				int height, const QString& imageType);
};
#endif		// __DIR_CACHE_H__
