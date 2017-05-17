//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/um/buffer/UmTxQueue.h"

Define_Module(UmTxQueue);

void UmTxQueue::fragment(cPacket* pkt)
{
    Enter_Method("fragment()");        // Direct Method Call
    take(pkt);                        // Take ownership

    int packetSize = pkt->getByteLength();
    LteRlcUm* lteRlc = check_and_cast<LteRlcUm *>(getParentModule()->getSubmodule("um"));
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
    LteRlcSdu* rlcPkt = check_and_cast<LteRlcSdu *>(pkt);

    EV << "UmTxBuffer : Processing packet " <<
    rlcPkt->getSnoMainPacket() << " with LCID: " <<
    lteInfo->getLcid() << "\n";

    // Calculate total number of fragments for this packet
    int totalFragments = packetSize / fragmentSize_ +
        ((packetSize % fragmentSize_) ? 1 : 0);

    // Create main fragment
    int fragmentSeqNum = 0;
    LteRlcPdu* fragment = new LteRlcPdu("lteRlcFragment");
    fragment->setTotalFragments(totalFragments);
    fragment->setSnoMainPacket(rlcPkt->getSnoMainPacket());

    // Original packet is encapsulated and size is changed to fragmentSize_
    fragment->encapsulate(rlcPkt);
    fragment->setByteLength(fragmentSize_);

    EV << "UmTxBuffer : " << totalFragments << " fragments created\n";

    // Send all fragments except the last one
    for (fragmentSeqNum = 0; fragmentSeqNum < totalFragments - 1; fragmentSeqNum++)
    {
        // FIXME: possible memory leak?
        LteRlcPdu* newFrag = fragment->dup();
        newFrag->setSnoFragment(fragmentSeqNum);
        newFrag->setControlInfo(lteInfo->dup());
        drop(newFrag);        // Drop ownership before sending through direct method call
        lteRlc->sendFragmented(newFrag);
    }
    // Send last fragment with different packet size
    fragment->setSnoFragment(fragmentSeqNum);
    fragment->setByteLength((packetSize % fragmentSize_) ? (packetSize % fragmentSize_) : fragmentSize_);
    fragment->setControlInfo(lteInfo);
    drop(fragment);        // Drop ownership before sending through direct method call
    lteRlc->sendFragmented(fragment);
    return;
}

int UmTxQueue::getFragmentSize()
{
    return fragmentSize_;
}

void UmTxQueue::setFragmentSize(int fragmentSize)
{
    fragmentSize_ = fragmentSize;
}

/*
 * Main functions
 */

void UmTxQueue::initialize()
{
    fragmentSize_ = par("fragmentSize");
    WATCH(fragmentSize_);
}
