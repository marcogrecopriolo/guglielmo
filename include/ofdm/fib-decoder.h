/*
 *    Copyright (C) 2021
 *    Marco Greco <marcogrecopriolo@gmail.com>
 *
 *    This file is part of the guglielmo FM DAB tuner software package.
 *
 *    guglielmo is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 of the License.
 *
 *    guglielmo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with guglielmo; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Taken from Qt-DAB with bug fixes and enhancements.
 *
 *    Copyright (C) 2013 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#ifndef FIB_DECODER_H
#define FIB_DECODER_H

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <cstdint>
#include <cstdio>
#include "msc-handler.h"
#include "dab-config.h"

class RadioInterface;

class ensembleDescriptor;
class dabConfig;
class Cluster;

class fibDecoder : public QObject {
    Q_OBJECT
  public:
    fibDecoder(RadioInterface *);
    ~fibDecoder();

    void clearEnsemble();
    bool syncReached();
    void dataforAudioService(const QString &, audiodata *);
    void dataforPacketService(const QString &, packetdata *, int16_t);
    int getSubChId(const QString &,  uint32_t);
    std::vector<serviceId> getServices(int);

    QString findService(uint32_t, int);
    void getParameters(const QString &, uint32_t *, int *);
    uint8_t get_ecc();
    int32_t get_ensembleId();
    QString get_ensembleName();
    int32_t get_CIFcount();
    void set_epgData(uint32_t, int32_t, const QString);
    std::vector<epgElement> get_timeTable(uint32_t);
    std::vector<epgElement> get_timeTable(const QString &);
    bool has_timeTable(uint32_t SId);
    std::vector<epgElement> find_epgData(uint32_t);

  protected:
    void process_FIB(uint8_t *);

  private:
    std::vector<serviceId> insert(std::vector<serviceId> l, serviceId n,
                                  int order);
    RadioInterface *myRadioInterface;
    dabConfig *currentConfig;
    dabConfig *nextConfig;
    ensembleDescriptor *ensemble;
    void process_FIG0(uint8_t *);
    void process_FIG1(uint8_t *);
    void FIG0Extension0(uint8_t *);
    void FIG0Extension1(uint8_t *);
    void FIG0Extension2(uint8_t *);
    void FIG0Extension3(uint8_t *);
    //	void FIG0Extension4(uint8_t *);
    void FIG0Extension5(uint8_t *);
    //	void FIG0Extension6(uint8_t *);
    void FIG0Extension7(uint8_t *);
    void FIG0Extension8(uint8_t *);
    void FIG0Extension9(uint8_t *);
    void FIG0Extension10(uint8_t *);
    //	void FIG0Extension11(uint8_t *);
    //	void FIG0Extension12(uint8_t *);
    void FIG0Extension13(uint8_t *);
    void FIG0Extension14(uint8_t *);
    //	void FIG0Extension15(uint8_t *);
    //	void FIG0Extension16(uint8_t *);
    void FIG0Extension17(uint8_t *);
    void FIG0Extension18(uint8_t *);
    void FIG0Extension19(uint8_t *);
    //	void FIG0Extension20(uint8_t *);
    void FIG0Extension21(uint8_t *);
    //	void FIG0Extension22(uint8_t *);
    //	void FIG0Extension23(uint8_t *);
    //	void FIG0Extension24(uint8_t *);
    //	void FIG0Extension25(uint8_t *);
    //	void FIG0Extension26(uint8_t *);

    int16_t HandleFIG0Extension1(uint8_t *, int16_t, uint8_t);
    int16_t HandleFIG0Extension2(uint8_t *, int16_t, uint8_t, uint8_t);
    int16_t HandleFIG0Extension3(uint8_t *, int16_t, uint8_t);
    int16_t HandleFIG0Extension5(uint8_t *, uint8_t, int16_t);
    int16_t HandleFIG0Extension8(uint8_t *, int16_t, uint8_t, uint8_t);
    int16_t HandleFIG0Extension13(uint8_t *, int16_t, uint8_t, uint8_t);
    int16_t HandleFIG0Extension21(uint8_t *, int16_t);

    void FIG1Extension0(uint8_t *);
    void FIG1Extension1(uint8_t *);
    //	void FIG1Extension2(uint8_t *);
    //	void FIG1Extension3(uint8_t *);
    void FIG1Extension4(uint8_t *);
    void FIG1Extension5(uint8_t *);
    void FIG1Extension6(uint8_t *);

    int findService(const QString &);
    int findService(uint32_t);
    void cleanupServiceList();
    void createService(QString name, uint32_t SId, int SCIds);

    int findServiceComponent(dabConfig *, int16_t);
    int findComponent(dabConfig *db, uint32_t SId, int16_t subChId);
    int findServiceComponent(dabConfig *, uint32_t, uint8_t);
    void bind_audioService(dabConfig *, int8_t, uint32_t, int16_t, int16_t,
                           int16_t, int16_t);
    void bind_packetService(dabConfig *, int8_t, uint32_t, int16_t, int16_t,
                            int16_t, int16_t);
    QString announcements(uint16_t);
    void setCluster(dabConfig *, int, int16_t, uint16_t);
    Cluster *getCluster(dabConfig *, int16_t);
    int32_t dateTime[8];
    QMutex fibLocker;
    int CIFcount;

  signals:
    void addToEnsemble(const QString &, uint);
    void nameOfEnsemble(int, const QString &);
    void ensembleLoaded(int);
    void clockTime(int, int, int, int, int);
    void changeinConfiguration();
    void startAnnouncement(const QString &, int);
    void stopAnnouncement(const QString &, int);
};
#endif
