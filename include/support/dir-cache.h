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
#include <QMap>
#include <QMultiHash>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QList>
#include <QPixmap>
#include <QRegularExpression>
#include <QDateTime>
#include <QVariant>

struct ImageInfo {
    QString path;
    int width;
    int height;
    QString imageType;
    QString hash;

    bool operator==(const ImageInfo& other) const {
	return width == other.width && 
	       height == other.height && 
	       imageType == other.imageType;
    }
};

struct EPGInfo {
    QString path;
    QDateTime startTime;
    QDateTime endTime;
    int hash;
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
    QStringList getEPGsForChannel(quint32 SId);
    QVariantMap loadEPG(const QString& path);
    bool addEPG(quint32 SId, const QDateTime& startTime, const QDateTime& endTime,
		int hash, const QVariantMap& epg);
    
private:
    QString cacheDir;
    QHash<QString, QString> cache;
    QMultiHash<QString, ImageInfo> channelIndex;
    QHash<QString, QList<EPGInfo>> epgIndex;
    
    void loadCache(void);
    void removeStaleEPGs(const QString& serviceId);
    void removeFromChannelIndex(const QString& serviceId, int width, 
				int height, const QString& imageType);
    QString hashKey(QString serviceId, int width, int height, const QString& imageType);
    QString calculateHash(const QByteArray& data);
};
#endif		// __DIR_CACHE_H__
