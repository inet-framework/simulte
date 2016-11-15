//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_INTERNETQUEUE_H_
#define _LTE_INTERNETQUEUE_H_

#include <omnetpp.h>

using namespace omnetpp;

/**
 * @class InternetQueue
 * @brief Transmission queue for finite bandwidth channels
 *
 * This class implements a queue used to transmit packet
 * over a finite bandwidth channel.
 *
 * When a packet is received from the input gate,
 * it is sent over the internet channel only if the channel
 * is not busy transmitting a previous packet.
 * If the channel is busy, the packet is stored in a queue.
 * The size of the queue is configurable ( 0 = infinite ).
 *
 */
class InternetQueue : public cSimpleModule
{
  protected:

    cQueue txQueue_;                     /// Transmission queue
    int queueSize_;                         /// Size of the transmission queue ( 0 = infinite)
    cMessage *endTransmissionEvent_;     /// Self message that notifies the end of a transmission
    cGate *queueOutGate_;                /// Output gate towards the datarate channel
    cChannel *datarateChannel_;          /// Datarate Channel

    // statistics
    int numSent_;       /// number of packets sent
    int numDropped_;    /// number of packets dropped

  public:

    /**
     * InternetQueue constructor.
     */
    InternetQueue();

    /**
     * InternetQueue destructor.
     */
    virtual ~InternetQueue();

  protected:

    /**
     * Initialize class fields.
     */
    virtual void initialize();

    /**
     * Handle messages.
     *
     * If the message is a self message that notifies the
     * end of a transmission, a new transmission can be started.
     * When a packet is received, if we are idle, it is transmitted
     * immediately, otherwise it is enqueued.
     * If the queue has a finite size and it is full, incoming packets
     * are dropped.
     *
     * @param msg message received
     */
    virtual void handleMessage(cMessage *msg);

    /**
     * Start the transmission of a new packet.
     *
     * The function sends the packet over the channel, then schedules
     * an endOfTransmissionEvent, to know when the channel
     * is idle again.
     *
     * @param msg packet received for transmission
     */
    virtual void startTransmitting(cPacket *pkt);

  private:

    /**
     * utility: show current statistics above the icon
     */
    void updateDisplayString();
};

#endif
