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
 *    Taken from Qt-DAB, with bug fixes and enhancements.
 *
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 */

#include "msc-handler.h"
#include "backend.h"
#include "constants.h"
#include "dab-params.h"
#include "logging.h"
#include "radio.h"

//	Interface program for processing the MSC.
//	The dabProcessor assumes the existence of an msc-handler, whether
//	a service is selected or not.

#define CUSize (4 * 16)
static int cifTable[] = {18, 72, 0, 36};

//	Note CIF counts from 0 .. 3
mscHandler::mscHandler(RadioInterface *mr, uint8_t dabMode,
                       RingBuffer<uint8_t> *frameBuffer)
    : params(dabMode), my_fftHandler(dabMode), myMapper(dabMode)
#ifdef __MSC_THREAD__
      ,
      bufferSpace(params.get_L())
#endif
{
    myRadioInterface = mr;
    this->frameBuffer = frameBuffer;
    cifVector.resize(55296);
    BitsperBlock = 2 * params.get_carriers();
    ibits.resize(BitsperBlock);
    nrBlocks = params.get_L();

    fft_buffer = my_fftHandler.getVector();
    phaseReference.resize(params.get_T_u());

    numberofblocksperCIF = cifTable[(dabMode - 1) & 03];
 
//	work_to_be_done. store (false);
#ifdef __MSC_THREAD__
    command.resize(nrBlocks);
    for (int i = 0; i < nrBlocks; i++)
        command[i].resize(params.get_T_u());
    amount = 0;
    running.store(false);
    start();
#endif
}

mscHandler::~mscHandler() {

//	work_to_be_done. store (false);
#ifdef __MSC_THREAD__
    running.store(false);
    while (isRunning())
        usleep(100);
#endif
    locker.lock();
    for (auto const &b : theBackends) {
        b->stopRunning();
        delete b;
    }
    locker.unlock();
    theBackends.resize(0);
}

//	Input is put into a buffer, a the code in a separate thread
//	will handle the data from the buffer
void mscHandler::processBlock_0(std::complex<float> *b) {
#ifdef __MSC_THREAD__
    bufferSpace.acquire(1);
    memcpy(command[0].data(), b,
           params.get_T_u() * sizeof(std::complex<float>));
    helper.lock();
    amount++;
    commandHandler.wakeOne();
    helper.unlock();
#else
    (void)b;
#endif
}

#ifdef __MSC_THREAD__
void mscHandler::process_Msc(std::complex<float> *b, int blkno) {

    // guard against bad blocks
    if (blkno < 0 || blkno >= nrBlocks)
        return;
    bufferSpace.acquire(1);
    memcpy(command[blkno].data(), b,
           params.get_T_u() * sizeof(std::complex<float>));
    helper.lock();
    amount++;
    commandHandler.wakeOne();
    helper.unlock();
}

void mscHandler::run() {
    int currentBlock = 0;

    if (running.load()) {
        log(LOG_DAB, LOG_MIN, "msc backend handler already running");
        return;
    }
    log(LOG_DAB, LOG_MIN, "msc backend handler thread starting");

    running.store(true);
    while (running.load()) {
        helper.lock();
        commandHandler.wait(&helper, 100);
        helper.unlock();
        while ((amount > 0) && running.load()) {
            memcpy(fft_buffer, command[currentBlock].data(),
                   params.get_T_u() * sizeof(std::complex<float>));

            //block 3 and up are needed as basis for demodulation the "mext"
            // block  "our" msc blocks start with blkno 4
            my_fftHandler.do_FFT();
            if (currentBlock >= 4) {
                for (int i = 0; i < params.get_carriers(); i++) {
                    int16_t index = myMapper.mapIn(i);
                    if (index < 0)
                        index += params.get_T_u();

                    std::complex<float> r1 =
                        fft_buffer[index] * conj(phaseReference[index]);
                    float ab1 = fastMagnitude(r1);

                    //      Recall:  the viterbi decoder wants 127 max pos, -
                    //      127 max neg we make the bits into softbits in the
                    //      range -127 .. 127
                    ibits[i] = -real(r1) / ab1 * 1023.0;
                    ibits[params.get_carriers() + i] = -imag(r1) / ab1 * 1023.0;
                }

                process_mscBlock(ibits, currentBlock);
            }
            memcpy(phaseReference.data(), fft_buffer,
                   params.get_T_u() * sizeof(std::complex<float>));
            bufferSpace.release(1);
            helper.lock();
            currentBlock = (currentBlock + 1) % (nrBlocks);
            amount -= 1;
            helper.unlock();
        }
    }
    log(LOG_DAB, LOG_MIN, "msc backend handler thread stopped");
}
#else
void mscHandler::process_Msc(std::complex<float> *b, int blkno) {
    if (blkno < 3)
        return;
    memcpy(fft_buffer, b, params.get_T_u() * sizeof(std::complex<float>));

    //	block 3 and up are needed as basis for demodulation the "mext" block
    //	"our" msc blocks start with blkno 4
    my_fftHandler.do_FFT();
    if (blkno >= 4) {
        for (int i = 0; i < params.get_carriers(); i++) {
            int16_t index = myMapper.mapIn(i);
            if (index < 0)
                index += params.get_T_u();
            std::complex<float> r1 =
                fft_buffer[index] * conj(phaseReference[index]);
            float ab1 = fastMagnitude(r1);

            //      Recall:  the viterbi decoder wants 127 max pos, - 127 max
            //      neg we make the bits into softbits in the range -127 .. 127
            ibits[i] = -real(r1) / ab1 * 256.0;
            ibits[params.get_carriers() + i] = -imag(r1) / ab1 * 256.0;
        }

        process_mscBlock(ibits, blkno);
    }
    memcpy(phaseReference.data(), fft_buffer,
           params.get_T_u() * sizeof(std::complex<float>));
}
#endif

//	Note, the set_Channel function is called from within a
//	different thread than the process_mscBlock method is,
//	so, a little bit of locking seems wise while
//	the actual changing of the settings is done in the
//	thread executing process_mscBlock
void mscHandler::reset_Buffers() {
    reset_Channel();
#ifdef __MSC_THREAD__
    running.store(false);
    while (isRunning())
        wait(100);
    bufferSpace.release(params.get_L() - bufferSpace.available());
    start();
#endif
}

void mscHandler::reset_Channel() {
    //	work_to_be_done. store (false);
    log(LOG_DAB, LOG_MIN,
        "msc backend hander channel reset: all %i services will be stopped", (int) theBackends.size());
    locker.lock();
    for (auto const &b : theBackends) {
        b->stopRunning();
        delete b;
    }
    theBackends.resize(0);
    locker.unlock();
}

void mscHandler::stopService(serviceDescriptor *d) {
    locker.lock();
    for (uint i = 0; i < theBackends.size(); i++) {
        Backend *b = theBackends.at(i);
        if (b->subChId == d->subchId) {
            log(LOG_DAB, LOG_MIN,
                "msc backend handler stopping (sub)service at subchannel %d",
                d->subchId);
            b->stopRunning();
            delete b;
            theBackends.erase(theBackends.begin() + i);
        }
    }
    //	if (theBackends. size () == 0)
    //	   work_to_be_done. store (false);
    log(LOG_DAB, LOG_MIN, "backends running: %i", (int) theBackends.size());
    locker.unlock();
}

bool mscHandler::set_Channel(serviceDescriptor *d,
                             RingBuffer<int16_t> *audioBuffer,
                             RingBuffer<uint8_t> *dataBuffer) {
    locker.lock();
    for (uint i = 0; i < theBackends.size(); i++) {
	log(LOG_DAB, LOG_CHATTY, "checking backend %i %i", i, theBackends.at(i)->serviceId);
        if (d->SId == theBackends.at(i)->serviceId) {
            log(LOG_DAB, LOG_MIN, "msc backend handler already running");
            locker.unlock();
            return false;
        }
    }
    theBackends.push_back(
        new Backend(myRadioInterface, d, audioBuffer, dataBuffer, frameBuffer));
    log(LOG_DAB, LOG_MIN, "backends running: %i", (int) theBackends.size());
    work_to_be_done.store(true);
    locker.unlock();
    return true;
}

//	add blocks. First is (should be) block 4, last is (should be)
//	nrBlocks -1.
//	Note that this method is called from within the ofdm-processor thread
//	while the set_xxx methods are called from within the
//	gui thread, so some locking is added
void mscHandler::process_mscBlock(std::vector<int16_t> fbits, int16_t blkno) {
    int16_t currentblk;

    currentblk = (blkno - 4) % numberofblocksperCIF;
    //	and the normal operation is:
    memcpy(&cifVector[currentblk * BitsperBlock], fbits.data(),
           BitsperBlock * sizeof(int16_t));
    if (currentblk < numberofblocksperCIF - 1)
        return;

    //	if (!work_to_be_done. load())
    //	   return;

    //	OK, now we have a full CIF and it seems there is some work to
    //	be done.  We assume that the backend itself
    //	does the work in a separate thread.
    locker.lock();
    for (auto const &b : theBackends) {
        int16_t startAddr = b->startAddr;
        int16_t Length = b->Length;
        if (Length > 0) { // Length = 0? should not happen
            _VLA(int16_t, temp, Length * CUSize);
            memcpy(temp, &cifVector[startAddr * CUSize],
                   Length * CUSize * sizeof(int16_t));
            (void)b->process(temp, Length * CUSize);
        }
    }
    locker.unlock();
}
