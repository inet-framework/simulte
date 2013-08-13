//
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteMacQueue.h"

LteMacQueue::LteMacQueue(int queueSize) :
    cPacketQueue("LteMacQueue")
{
    queueSize_ = queueSize;
}

LteMacQueue::LteMacQueue(const LteMacQueue& queue)
{
    operator=(queue);
}

LteMacQueue& LteMacQueue::operator=(const LteMacQueue& queue)
{
    cPacketQueue::operator=(queue);
    queueSize_ = queue.queueSize_;
    return *this;
}

LteMacQueue* LteMacQueue::dup() const
{
    return new LteMacQueue(*this);
}

// ENQUEUE
bool LteMacQueue::pushBack(cPacket *pkt)
{
    if (getByteLength() + pkt->getByteLength() > queueSize_ && queueSize_ != 0)
        return false;  // Packet Queue Full

    cPacketQueue::insert(pkt);
    return true;
}

bool LteMacQueue::pushFront(cPacket *pkt)
{
    if (getByteLength() + pkt->getByteLength() > queueSize_ && queueSize_ != 0)
        return false; // Packet Queue Full

    cPacketQueue::insertBefore(cPacketQueue::front(), pkt);
    return true;
}

cPacket* LteMacQueue::popFront()
{
    return getQueueLength() > 0 ? cPacketQueue::pop() : NULL;
}

cPacket* LteMacQueue::popBack()
{
    return getQueueLength() > 0 ? cPacketQueue::remove(cPacketQueue::back()) : NULL;
}

simtime_t LteMacQueue::getHolTimestamp() const
{
    return getQueueLength() > 0 ? cPacketQueue::front()->getTimestamp() : 0;
}

int64_t LteMacQueue::getQueueOccupancy() const
{
    return cPacketQueue::getByteLength();
}

int64_t LteMacQueue::getQueueSize() const
{
    return queueSize_;
}

int LteMacQueue::getQueueLength() const
{
    return cPacketQueue::getLength();
}

std::ostream &operator << (std::ostream &stream, const LteMacQueue* queue)
{
    stream << "LteMacQueue-> Length: " << queue->getQueueLength() <<
        " Occupancy: " << queue->getQueueOccupancy() <<
        " HolTimestamp: " << queue->getHolTimestamp() <<
        " Size: " << queue->getQueueSize();
    return stream;
}
