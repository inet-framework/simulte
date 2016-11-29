//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/am/buffer/AmRxQueue.h"
#include "stack/rlc/am/LteRlcAm.h"
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/mac/layer/LteMacBase.h"

Define_Module(AmRxQueue);

unsigned int AmRxQueue::totalCellRcvdBytes_ = 0;

AmRxQueue::AmRxQueue() :
    timer_(this)
{
    rxWindowDesc_.firstSeqNum_ = 0;
    rxWindowDesc_.seqNum_ = 0;
    lastSentAck_ = 0;
    firstSdu_ = 0;

    // in order create a back connection (AM CTRL) , a flow control
    // info for sending ctrl messages to tx entity is required

    flowControlInfo_ = NULL;

    timer_.setTimerId(BUFFERSTATUS_T);
}

void AmRxQueue::initialize()
{
    //  loading parameters from NED
    rxWindowDesc_.windowSize_ = par("rxWindowSize");
    ackReportInterval_ = par("ackReportInterval");
    statusReportInterval_ = par("statusReportInterval");

    discarded_.resize(rxWindowDesc_.windowSize_);
    received_.resize(rxWindowDesc_.windowSize_);
    totalRcvdBytes_ = 0;

    cModule* parent = check_and_cast<LteRlcAm*>(
        getParentModule()->getSubmodule("am"));
    //statistics
    LteMacBase* mac = check_and_cast<LteMacBase*>(
        getParentModule()->getParentModule()->getSubmodule("mac"));

    if (mac->getNodeType() == ENODEB)
    {
        rlcCellPacketLoss_ = parent->registerSignal("rlcCellPacketLossUl");
        rlcPacketLoss_ = parent->registerSignal("rlcPacketLossUl");
        rlcPduPacketLoss_ = parent->registerSignal("rlcPduPacketLossUl");
        rlcDelay_ = parent->registerSignal("rlcDelayUl");
        rlcThroughput_ = parent->registerSignal("rlcThroughputUl");
        rlcPduDelay_ = parent->registerSignal("rlcPduDelayUl");
        rlcPduThroughput_ = parent->registerSignal("rlcPduThroughputUl");
        rlcCellThroughput_ = parent->registerSignal("rlcCellThroughputUl");
    }
    else
    {
        cModule* nodeB = getRlcByMacNodeId(mac->getMacCellId(), UM);
        rlcPacketLoss_ = parent->registerSignal("rlcPacketLossDl");
        rlcPduPacketLoss_ = parent->registerSignal("rlcPduPacketLossDl");
        rlcDelay_ = parent->registerSignal("rlcDelayDl");
        rlcThroughput_ = parent->registerSignal("rlcThroughputDl");
        rlcPduDelay_ = parent->registerSignal("rlcPduDelayDl");
        rlcPduThroughput_ = parent->registerSignal("rlcPduThroughputDl");
        rlcCellThroughput_ = nodeB->registerSignal("rlcCellThroughputDl");
        rlcCellPacketLoss_ = nodeB->registerSignal("rlcCellPacketLossDl");
    }
}

void AmRxQueue::handleMessage(cMessage* msg)
{
    if (!(msg->isSelfMessage()))
        throw cRuntimeError("Unexpected message received from AmRxQueue");

    EV << NOW << "AmRxQueue::handleMessage timer event received, sending status report " << endl;

    // signal timer event handled
    timer_.handle();

    // Send status report to the AM Tx entity
    sendStatusReport();

    // Reschedule the timer if there are PDUs in the buffer

    for (unsigned int i = 0; i < rxWindowDesc_.windowSize_; i++)
    {
        if (pduBuffer_.get(i) != 0)
        {
            timer_.start(statusReportInterval_);
            break;
        }
    }
}

void AmRxQueue::discard(const int sn)
{
    int index = sn - rxWindowDesc_.firstSeqNum_;

    if ((index < 0) || (index >= rxWindowDesc_.windowSize_))
        throw cRuntimeError("AmRxQueue::discard PDU %d out of rx window ", sn);

    int discarded = 0;

    Direction dir = UNKNOWN_DIRECTION;

    MacNodeId dstId = 0, srcId = 0;

    for (int i = 0; i <= index; ++i)
    {
        discarded_.at(i) = true;

        if (pduBuffer_.get(i) != NULL)
        {
            LteRlcAmPdu* pdu = check_and_cast<LteRlcAmPdu*>(pduBuffer_.remove(i));
            FlowControlInfo* ci = check_and_cast<FlowControlInfo*>(pdu->getControlInfo());
            dir = (Direction) ci->getDirection();
            dstId = ci->getDestId();
            srcId = ci->getSourceId();
            delete pdu;
            ++discarded;
        }
        else
            throw cRuntimeError("AmRxQueue::discard PDU at position %d already discarded", i);
    }

    EV << NOW << " AmRxQueue::discard , discarded " << discarded << " PDUs " << endl;

    if (dir != UNKNOWN_DIRECTION)
    {
        // UE module
        cModule* ue = getRlcByMacNodeId((dir == DL ? dstId : srcId), UM);
        // NODEB
        cModule* nodeb = getRlcByMacNodeId((dir == DL ? srcId : dstId), UM);

        ue->emit(rlcPacketLoss_, 1.0);
        nodeb->emit(rlcCellPacketLoss_, 1.0);
    }
}

void AmRxQueue::enque(LteRlcAmPdu* pdu)
{
    Enter_Method("enque()");

    LteRlcAm* lteRlc = check_and_cast<LteRlcAm *>(
        getParentModule()->getSubmodule("am"));

    // Check if a control PDU has been received
    if (pdu->getAmType() != DATA)
    {
        if ((pdu->getAmType() == ACK))
        {
            EV << NOW << " AmRxQueue::enque Received ACK message" << endl;
            // forwarding ACK to associated transmitting entity
            lteRlc->routeControlMessage(pdu);
            return;
        }

        else if ((pdu->getAmType() == MRW))
        {
            EV << NOW << " AmRxQueue::enque MRW command [" <<
            pdu->getSnoMainPacket()
               << "] received for sequence number  " << pdu->getLastSn() << endl;
            // Move the receiver window
            moveRxWindow(pdu->getLastSn());
            // ACK This message
            LteRlcAmPdu * mrw = new LteRlcAmPdu("rlcAmPdu");
            mrw->setAmType(MRW_ACK);
            mrw->getSnoMainPacket();
            mrw->setSnoMainPacket(pdu->getSnoMainPacket());
            mrw->setSnoFragment(rxWindowDesc_.firstSeqNum_);

            // todo setting AM ctrl byte size
            mrw->setByteLength(RLC_HEADER_AM);
            // set flowcontrolinfo
            mrw->setControlInfo(flowControlInfo_->dup());
            // sending control PDU
            lteRlc->sendFragmented(mrw);
            EV << NOW << " AmRxQueue::enque sending MRW_ACK message " << pdu->getSnoMainPacket() << endl;
        }
        else if ((pdu->getAmType() == MRW_ACK))
        {
            EV << NOW << " MRW ACK routing to TX entity " << endl;
            // route message to reverse link TX entity
            lteRlc->routeControlMessage(pdu);
            return;
        }
        else
        {
            throw cRuntimeError("RLC AM - Unknown status PDU");
        }
        return;
    }

            //If the timer has not been enabled yet, start the timer to handle status report (ACK) messages

    if (timer_.idle())
    {
        EV << NOW << " AmRxQueue::enque reporting timer was idle, will fire at " << NOW.dbl() + statusReportInterval_.dbl() << endl;
        timer_.start(statusReportInterval_);
    }
    else
    {
        EV << NOW << " AmRxQueue::enque reporting timer was already on, will fire at " << NOW.dbl() + timer_.remaining().dbl() << endl;
    }

        // check if we need to extract FlowControlInfo for building up the matrix
    if (flowControlInfo_ == NULL)
    {
        FlowControlInfo* orig = check_and_cast<FlowControlInfo*>(
            pdu->getControlInfo());
        // make a copy of original control info
        flowControlInfo_ = orig->dup();
        // swap source and destination fields
        flowControlInfo_->setSourceId(orig->getDestId());
        flowControlInfo_->setSrcPort(orig->getDstPort());
        flowControlInfo_->setSrcAddr(orig->getDstAddr());
        flowControlInfo_->setDestId(orig->getSourceId());
        flowControlInfo_->setDstPort(orig->getSrcPort());
        flowControlInfo_->setDstAddr(orig->getDstAddr());

        // set up other field
        flowControlInfo_->setDirection((orig->getDirection() == DL) ? UL : DL);
    }

    // Get the RLC PDU Transmission sequence number
    int tsn = pdu->getSnoFragment();

    // Get the index of the PDU buffer
    int index = tsn - rxWindowDesc_.firstSeqNum_;

    if (index < 0)
    {
        EV << NOW << " AmRxQueue::enque the received PDU with " << index << " was already buffered and discarded by MRW" << endl;
        delete pdu;
    }
    else if ((index >= rxWindowDesc_.windowSize_))
    {
        // The received PDU is out of the window
        throw cRuntimeError("AmRxQueue::enque(): received PDU with position %d out of the window of size %d",index,rxWindowDesc_.windowSize_);
    }
    else
    {
        // Check if the tsn is the next expected TSN.
        if (tsn == rxWindowDesc_.seqNum_)
        {
            rxWindowDesc_.seqNum_++;

            EV << NOW << "AmRxQueue::enque DATA PDU received at index [" << index << "] with fragment number [" << tsn << "] in sequence " << endl;
        }
        else
        {
            rxWindowDesc_.seqNum_ = tsn + 1;

            EV << NOW << "AmRxQueue::enque DATA PDU received at index [" << index << "] with fragment number ["
               << tsn << "] out of sequence, sending status report " << endl;
            sendStatusReport();
        }

        // Check if the PDU has already been received

        if (received_.at(index) == true)
        {
            EV << NOW << " AmRxQueue::enque the received PDU has index " << index << " which points to an already busy location" << endl;

            // Check if the received PDU points
            // to the same data structure of the PDU
            // stored in the buffer

            LteRlcAmPdu* bufferedpdu = check_and_cast<LteRlcAmPdu*>( pduBuffer_.get(index));

            if (bufferedpdu->getSnoMainPacket() == pdu->getSnoMainPacket())
            {
                // Drop the incoming RLC PDU
                EV << NOW << " AmRxQueue::enque the received PDU with " << index << " was already buffered " << endl;
                delete pdu;
            }
            else
            {
                throw cRuntimeError("AmRxQueue::enque(): the received PDU at position %d"
                    "main SDU %d overlaps with an old one , main SDU %d",index,pdu->getSnoMainPacket(),
                    bufferedpdu->getSnoMainPacket());
            }
        }
        else
        {
            // Buffer the PDU
            pduBuffer_.addAt(index, pdu);
            received_.at(index) = true;
            // Check if this PDU forms a complete SDU
            checkCompleteSdu(index);
        }
    }
}

void AmRxQueue::passUp(const int index)
{
    Enter_Method("passUp");

    LteRlcAm* lteRlc = check_and_cast<LteRlcAm *>(getParentModule()->getSubmodule("am"));

    // duplicate buffered PDU. We cannot detach it from receiver window until a move Rx command is executed.
    LteRlcAmPdu* bufferedpdu = (check_and_cast<LteRlcAmPdu*>(pduBuffer_.get(index)))->dup();

    EV << NOW << " AmRxQueue::passUp passing up SDU[" << bufferedpdu->getSnoMainPacket() << "] referenced by PDU at position " << index << endl;

    // duplicate buffered PDU control info too.
    FlowControlInfo * ci = check_and_cast<FlowControlInfo*>(
        (check_and_cast<LteRlcAmPdu*>(pduBuffer_.get(index)))->getControlInfo()->dup());

    int origPktSize = bufferedpdu->getEncapsulatedPacket()->getEncapsulatedPacket()->getByteLength();

    EV << NOW << " AmRxQueue::passUp original packet size  " << origPktSize << " pdu size " << bufferedpdu->getByteLength() << endl;

    bufferedpdu->setByteLength(origPktSize); // Set original packet size before decapsulating
    bufferedpdu->getEncapsulatedPacket()->setByteLength(origPktSize);

    cPacket* pkt = bufferedpdu->decapsulate()->decapsulate();

    // cleanup duped PDU.
    delete bufferedpdu;

    Direction dir = (Direction) ci->getDirection();
    MacNodeId dstId = ci->getDestId();
    MacNodeId srcId = ci->getSourceId();
    cModule* nodeb = NULL;
    cModule* ue = NULL;
    double delay = (NOW - pkt->getCreationTime()).dbl();
    pkt->setControlInfo(ci);

    if (dir == DL)
    {
        nodeb = getRlcByMacNodeId(srcId, UM);
        ue = getRlcByMacNodeId(dstId, UM);
    }
    else if (dir == UL)
    {
        nodeb = getRlcByMacNodeId(dstId, UM);
        ue = getRlcByMacNodeId(srcId, UM);
    }

    totalRcvdBytes_ += pkt->getByteLength();
    totalCellRcvdBytes_ += pkt->getByteLength();
    double tputSample = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    double cellTputSample = (double)totalCellRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

    nodeb->emit(rlcCellThroughput_, cellTputSample);
    ue->emit(rlcThroughput_, tputSample);
    ue->emit(rlcDelay_, delay);
    ue->emit(rlcPacketLoss_, 0.0);
    nodeb->emit(rlcCellPacketLoss_, 0.0);
    // Get the SDU and pass it to the upper layers - PDU // SDU // PDCPPDU
    lteRlc->sendDefragmented(pkt);
    // send buffer status report
    sendStatusReport();
}

void AmRxQueue::checkCompleteSdu(const int index)
{
    LteRlcAmPdu* pdu = check_and_cast<LteRlcAmPdu*>(pduBuffer_.get(index));
    int incomingSdu = pdu->getSnoMainPacket();

    EV << NOW << " AmRxQueue::checkCompleteSdu at position " << index << " for SDU number " << incomingSdu << endl;

    if (firstSdu_ == -1)
    {
        // this is the first PDU in Rx window of new SDU
        firstSdu_ = incomingSdu;
    }

    // complete SDU found flag
    bool complete = false;
    // backward search OK flag
    bool bComplete = false;
    LteRlcAmPdu* tempPdu = NULL;
    int tempSdu = -1;
    // Index of the first PDU related to the SDU
    int firstIndex = -1;
    // If the RLC PDU is whole pass the RLC PDU to the upper layer
    if (pdu->isWhole())
    {
        EV << NOW << " AmRxQueue::checkCompleteSdu - complete SDU has been found (PDU at " << index << " was whole)" << endl;
        passUp(index);
        return;
    }
    else
    {
        if (!pdu->isFirst())
        {
            if ((index) == 0)
            {
                // We are at the beginning of the buffer and PDU is in the middle of its SDU sequence.
                // Check if the previous PDU of this SDU have been correctly received.
                if (firstSdu_ == incomingSdu)
                {
                    firstIndex = index;
                    bComplete = true;
                }
                else
                throw cRuntimeError("AmRxQueue::checkCompleteSdu(): first SDU error : %d",firstSdu_);
            }
            else
            {
                // check for previous PDUs
                for (int i = index - 1; i >= 0; i--)
                {
                    if (received_.at(i) == false)
                    {
                        // There is NO RLC PDU in this position
                        // The SDU is not complete
                        EV << NOW << " AmRxQueue::checkCompleteSdu: SDU cannot be reconstructed, no PDU received at positions earlier than " << i << endl;
                        return;
                    }
                    else
                    {
                        tempPdu = check_and_cast<LteRlcAmPdu*>(pduBuffer_.get(i));
                        tempSdu = tempPdu->getSnoMainPacket();

                        if (tempSdu != incomingSdu)
                        throw cRuntimeError("AmRxQueue::checkCompleteSdu(): backward search: fragmentation error : the receiver buffer contains parts of different SDUs, PDU seqnum %d",pdu->getSnoFragment());

                        if (tempPdu->isFirst())
                        {
                            // found starting PDU
                            firstIndex = i;
                            bComplete = true;
                            break;
                        }
                        else if (tempPdu->isLast()
                            || tempPdu->isWhole())
                        {
                            throw cRuntimeError("AmRxQueue::checkCompleteSdu(): backward search: sequence error, found last or whole PDU [%d] preceding a middle one [%d], belonging to  SDU [%d], current SDU is [%d]",tempPdu->getSnoFragment(),(check_and_cast<LteRlcAmPdu*>(
                                        pduBuffer_.get(i+1)))->getSnoFragment(),(check_and_cast<LteRlcAmPdu*>(
                                        pduBuffer_.get(i+1)))->getSnoMainPacket(),tempSdu);
                        }
                    }
                }
                firstIndex=0;
                // all PDUs received
                bComplete=true;
            }
        }
        else
        {
            // since PDU is the first one, backward search is automatically completed by definition
            bComplete = true;
        }
    }
    if (!bComplete)
    {
        EV << NOW
           << " AmRxQueue::checkCompleteSdu - SDU cannot be reconstructed, backward search didn't found any predecessors to PDU at "
           << index << endl;
        return;
    }
        // search ended with current PDU, the whole SDU can be reconstructed.
    if (pdu->isLast())
    {
        EV << NOW << " AmRxQueue::checkCompleteSdu - complete SDU has been found, backward search was successful, and current was last of its SDU"
        " passing up " << firstIndex << endl;
        passUp(firstIndex);
        return;
    }

    EV << NOW << " AmRxQueue::checkCompleteSdu initiating forward search, staring from position " << index + 1 << endl;
    // Go forward looking for remaining  PDUs

    for (int i = index + 1; i < (rxWindowDesc_.windowSize_); ++i)
    {
        if (received_.at(i) == false)
        {
            EV << NOW << " AmRxQueue::checkCompleteSdu forward search failed, no PDU at position " << i << " corresponding to"
            " SN  " << i+rxWindowDesc_.firstSeqNum_ << endl;

            // There is NO RLC PDU in this position
            // The SDU is not complete
            return;
        }
        else
        {
            tempPdu = check_and_cast<LteRlcAmPdu*>(pduBuffer_.get(i));
            tempSdu = tempPdu->getSnoMainPacket();
            if (tempSdu != incomingSdu)
            throw cRuntimeError("AmRxQueue::checkCompleteSdu(): SDU numbers differ from position %d to %d : former SDU %d second %d",i,i-1,incomingSdu,tempSdu);
        }
        if (tempPdu->isLast())
        {
            complete = true;
            EV << NOW << " AmRxQueue::checkCompleteSdu: forward search successful, last PDU found at position "
               << i << endl;

            break;
        }
        else if (tempPdu->isFirst() || tempPdu->isWhole())
        {
            throw cRuntimeError("AmRxQueue::checkCompleteSdu(): forward search: PDU sequencer error ");
            break;
        }
    }
            // all PDU received
    complete = true;

    if (complete == true)
    {
        EV << NOW << " AmRxQueue::checkCompleteSdu - complete SDU has been found after forward search , passing up "
           << firstIndex << endl;
        passUp(firstIndex);
        return;
    }
}

void AmRxQueue::sendStatusReport()
{
    Enter_Method("sendStatusReport()");
    EV << NOW << " AmRxQueue::sendStatusReport " << endl;

    // Check if the prohibit status report has been set.

    if ((NOW.dbl() - lastSentAck_.dbl()) < ackReportInterval_.dbl())
    {
        EV << NOW
           << " AmRxQueue::sendStatusReport , minimum interval not reached "
           << ackReportInterval_.dbl() << endl;

        return;
    }

        // Compute cumulative ACK
    int cumulative = 0;
    bool hole = !received_.at(0);
    std::vector<bool> bitmap;

    for (int i = 0; i < rxWindowDesc_.windowSize_; ++i)
    {
        if ((received_.at(i) == true) && !hole)
        {
            cumulative++;
        }
        else if ((cumulative > 0) || hole)
        {
            hole = true;
            bitmap.push_back(received_.at(i));
        }
    }

    // The BitMap :
    // Starting from the cumulative ACK the next received PDU
    // are stored.

    EV << NOW << " AmRxQueue::sendStatusReport : cumulative ACK value "
       << cumulative << " bitmap lenght " << bitmap.size() << endl;

    if ((cumulative > 0) || bitmap.size() > 0)
    {
        // create a new RLC PDU
        LteRlcAmPdu * pdu = new LteRlcAmPdu("rlcAmPdu");
        // set RLC type descriptor
        pdu->setAmType(ACK);

        int lastSn = rxWindowDesc_.firstSeqNum_ + cumulative - 1;

        pdu->setLastSn(lastSn);
        // set bitmap
        EV << NOW << " AmRxQueue::sendStatusReport : sending the cumulative ACK for  "
           << lastSn << endl;

        // Note that, FSN could be out of the receiver windows, in this case
        // no ACK BITMAP message is sent.
        if (cumulative < rxWindowDesc_.windowSize_)
        {
            pdu->setBitmapVec(bitmap);
            // Start the BITMAP ACK report at the end of the cumulative ACK
            int fsn = rxWindowDesc_.firstSeqNum_ + cumulative;
            // We set the first sequence number of the BITMAP starting
            // from the end of the cumulative ACK message.
            pdu->setFirstSn(fsn);
        }
        // todo setting byte size
        pdu->setByteLength(RLC_HEADER_AM);
        // set flowcontrolinfo
        pdu->setControlInfo(flowControlInfo_->dup());
        // sending control PDU
        LteRlcAm* lteRlc = check_and_cast<LteRlcAm *>(
            getParentModule()->getSubmodule("am"));
        lteRlc->sendFragmented(pdu);
        lastSentAck_ = NOW;
    }
    else
    {
        EV << NOW
           << " AmRxQueue::sendStatusReport : NOT sending the cumulative ACK for  "
           << cumulative << endl;
    }
}

int AmRxQueue::computeWindowShift() const
{
    EV << NOW << "AmRxQueue::computeWindowShift" << endl;
    int shift = 0;
    for ( int i = 0; i < rxWindowDesc_.windowSize_; ++i)
    {
        if (received_.at(i) == true || discarded_.at(i) == true)
        {
            ++shift;
        }
        else
        {
            break;
        }
    }
    return shift;
}

void AmRxQueue::moveRxWindow(const int seqNum)
{
    EV << NOW << " AmRxQueue::moveRxWindow moving forth to match first seqnum " << seqNum << endl;

    int pos = seqNum - rxWindowDesc_.firstSeqNum_;

    if (pos <= 0)
    return;  // ignore the shift , it is uneffective.

    if (pos>rxWindowDesc_.windowSize_)
    throw cRuntimeError("AmRxQueue::moveRxWindow(): positions %d win size %d , seq num %d",pos,rxWindowDesc_.windowSize_,seqNum);

    LteRlcAmPdu * pdu = NULL;
    // Shift the window and check if a complete SDU is shifted or if
    // part of an SDU is still in the buffer

    int currentSdu = firstSdu_;
    int firstSeqNum=0;

    EV << NOW << " AmRxQueue::moveRxWindow current SDU is " << firstSdu_ << endl;

    for ( int i = 0; i < pos; ++i)
    {
        if (pduBuffer_.get(i) != NULL)
        {
            pdu = check_and_cast<LteRlcAmPdu*>(pduBuffer_.remove(i));
            currentSdu = (pdu->getSnoMainPacket());

            if (pdu->isLast() || pdu->isWhole())
            {
                // Reset the last PDU seen.
                currentSdu = -1;
            }
            delete pdu;
        }
        else
        {
            currentSdu = -1;
        }
    }

    for ( int i = pos; i < rxWindowDesc_.windowSize_; ++i)
    {
        if (pduBuffer_.get(i) != NULL)
        {
            pduBuffer_.addAt(i-pos, pduBuffer_.remove(i));
        }
        else
        {
            pduBuffer_.remove(i);
        }
        received_.at(i-pos) = received_.at(i);
        discarded_.at(i-pos) = discarded_.at(i);
        received_.at(i) = false;
        discarded_.at(i) = false;
    }

    rxWindowDesc_.firstSeqNum_ += pos;

    EV << NOW << " AmRxQueue::moveRxWindow first sequence number updated to "
       << rxWindowDesc_.firstSeqNum_ << endl;

    firstSdu_ = currentSdu;
    EV << NOW << " AmRxQueue::moveRxWindow current SDU updated to "
       << firstSdu_ << endl;
}

AmRxQueue::~AmRxQueue()
{
}

