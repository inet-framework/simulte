//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/lteip/InternetQueue.h"

Define_Module(InternetQueue);

InternetQueue::InternetQueue()
{
    endTransmissionEvent_ = NULL;
    datarateChannel_ = NULL;
    queueOutGate_ = NULL;
}

InternetQueue::~InternetQueue()
{
    cancelAndDelete(endTransmissionEvent_);
}

void InternetQueue::initialize()
{
    // create self message for end of transmission event
    endTransmissionEvent_ = new cMessage("EndTxEvent");

    // read queue size
    queueSize_ = par("queueSize");
    queueSize_ = (queueSize_ > 0) ? queueSize_ : 0; // 0 means infinite

    // initialize statistics
    numSent_ = numDropped_ = 0;

    // watches
    WATCH(numSent_);
    WATCH(numDropped_);
}

void InternetQueue::handleMessage(cMessage *msg)
{
    // queue not connected
    if (datarateChannel_ == NULL)
    {
        // remember the output gate now, to speed up send()
        queueOutGate_ = gate("internetChannelOut");

        // we're connected if other end of connection path is an input gate
        bool connected = (queueOutGate_->getPathEndGate()->getType()
            == cGate::INPUT);

        // if we're connected, get the channel
        datarateChannel_ =
            connected ? queueOutGate_->getTransmissionChannel() : NULL;

        if (!connected)
        {
            EV << "Queue is not connected, dropping packet " << msg << endl;
            delete msg;
            numDropped_++;
        }
        else
        {
            EV << "Queue size: " << queueSize_ << endl;
            EV << "Queue connected: " << connected << endl;
        }
    }

    // self message: end of previous transmission
    if (msg == endTransmissionEvent_)
    {
        // Transmission finished, we can start next one.
        numSent_++;
        EV << "Transmission finished" << endl;
        if (!txQueue_.isEmpty())
        {
            cPacket *pk = (cPacket *) txQueue_.pop();
            startTransmitting(pk);
        }
    }

    // packet from LteIp module arrived on gate "lteIPIn"
    else
    {
        if (endTransmissionEvent_->isScheduled())
        {
            // We are currently busy, so just queue up the packet
            EV << "Received " << msg
               << " for transmission but transmitter busy, queuing." << endl;

            // if the queue has a finite length and it is full, drop the packet
            if (queueSize_ && txQueue_.getLength() >= queueSize_)
            {
                EV << "The transmission queue is full. Packet dropped" << endl;
                numDropped_++;
                delete msg;
            }
            else
                txQueue_.insert(msg);
        }
        else
        {
            // We are idle, so we can start transmitting right away.
            EV << "Received " << msg << " for transmission\n";
            startTransmitting(check_and_cast<cPacket *>(msg));
        }
    }

    if (getEnvir()->isGUI())
    updateDisplayString();
}

void InternetQueue::startTransmitting(cPacket *pkt)
{
    // Send
    EV << "Starting transmission of " << pkt << endl;
    send(pkt, queueOutGate_);

    // Schedule an event for the time when last bit will leave the gate.
    simtime_t endTransmissionTime =
    datarateChannel_->getTransmissionFinishTime();
    scheduleAt(endTransmissionTime, endTransmissionEvent_);
    EV << "The transmission will finish at simtime: " << endTransmissionTime << endl;
}

void InternetQueue::updateDisplayString()
{
    char buf[80] = "";
    if (numSent_ > 0)
        sprintf(buf + strlen(buf), "sent:%d ", numSent_);
    if (numDropped_ > 0)
        sprintf(buf + strlen(buf), "drop:%d ", numDropped_);
    getDisplayString().setTagArg("t", 0, buf);
}

