//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <climits>
#include "stack/mac/buffer/LteMacQueue.h"

LteMacQueue::LteMacQueue(int queueSize) :
    cPacketQueue("LteMacQueue")
{
    queueSize_ = queueSize;
    lastUnenqueueableMainSno = UINT_MAX;
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
    if (!isEnqueueablePacket(pkt))
         return false; // packet queue full or we have discarded fragments for this main packet

    cPacketQueue::insert(pkt);
    return true;
}

bool LteMacQueue::pushFront(cPacket *pkt)
{
    if (!isEnqueueablePacket(pkt))
        return false; // packet queue full or we have discarded fragments for this main packet

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

bool LteMacQueue::isEnqueueablePacket(cPacket* pkt){
    LteRlcPdu_Base* pdu = dynamic_cast<LteRlcPdu_Base *>(pkt);
    if(queueSize_ == 0){
        // unlimited queue size -- nothing to check for
        return true;
    }
    if(pdu != NULL){
        if(pdu->getTotalFragments() > 1) {
            bool allFragsWillFit = ((pdu->getTotalFragments()-pdu->getSnoFragment())*pdu->getByteLength() + getByteLength() < queueSize_);
            bool enqueable = (pdu->getSnoMainPacket() != lastUnenqueueableMainSno) && allFragsWillFit;
            if(allFragsWillFit && !enqueable){
                EV_DEBUG << "PDU would fit but discarded frags before - rejecting fragment: " << pdu->getSnoMainPacket() << ":" << pdu->getSnoFragment() << std::endl;
            }
            if(!enqueable){
                lastUnenqueueableMainSno = pdu->getSnoMainPacket();
            }
            return enqueable;
        }
    } else {
        EV_WARN << "LteMacQueue: cannot estimate remaining fragments - unknown packet type " << pkt->getFullName() << std::endl;
    }
    // no fragments or unknown type -- can always be enqueued if there is enough space in the queue
    return (pkt->getByteLength() + getByteLength() < queueSize_);
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
