//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/am/buffer/AmTxQueue.h"
#include "stack/rlc/am/LteRlcAm.h"
#include "stack/mac/layer/LteMacBase.h"

Define_Module(AmTxQueue);

AmTxQueue::AmTxQueue() :
    pduTimer_(this), mrwTimer_(this), bufferStatusTimer_(this)
{
    currentSdu_ = NULL;

    lteInfo_ = NULL;
    //initialize timer IDs
    pduTimer_.setTimerId(PDU_T);
    mrwTimer_.setTimerId(MRW_T);
    bufferStatusTimer_.setTimerId(BUFFER_T);
}

void AmTxQueue::initialize()
{
    // initialize all parameters from NED.
    maxRtx_ = par("maxRtx");
    fragDesc_.fragUnit_ = par("fragmentSize");
    pduRtxTimeout_ = par("pduRtxTimeout");
    ctrlPduRtxTimeout_ = par("ctrlPduRtxTimeout");
    bufferStatusTimeout_ = par("bufferStatusTimeout");
    txWindowDesc_.windowSize_ = par("txWindowSize");
    // resize status vectors
    received_.resize(txWindowDesc_.windowSize_, false);
    discarded_.resize(txWindowDesc_.windowSize_, false);
}

AmTxQueue::~AmTxQueue()
{
}

void AmTxQueue::enque(LteRlcAmSdu* sdu)
{
    EV << NOW << " AmTxQueue::enque - inserting new SDU  " << endl;
    // Buffer the SDU
    sduQueue_.insert(sdu);

    // Check if there are waiting SDUs
    if (currentSdu_ == NULL)
    {
        // Add AM-PDU to the transmission buffer
        if (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_ < txWindowDesc_.windowSize_)
        {
            // RLC AM PDUs can be added to the buffer
            addPdus();
        }
    }
}

void AmTxQueue::addPdus()
{
    Enter_Method("addPdus()");

    // Add PDUs to the AM transmission buffer until the transmission
    // window is full or until the SDU buffer is empty
    unsigned int addedPdus = 0;

    while ((txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_) < txWindowDesc_.windowSize_)
    {
        if (currentSdu_ == NULL && sduQueue_.isEmpty())
        {
            // No data to send

            EV << NOW << " AmTxQueue::addPdus - No data to send " << endl;
            break;
        }

        // Check if we can start to fragment a new SDU
        if (currentSdu_ == NULL)
        {
            EV << NOW << " AmTxQueue::addPdus - No pending SDU has been found" << endl;
            // Get the first available SDU (buffer has already been check'd being non empty)
            currentSdu_ = check_and_cast<LteRlcAmSdu*>(sduQueue_.pop());

            // Starting Fragmentation
            fragDesc_.startFragmentation(currentSdu_->getByteLength(), txWindowDesc_.seqNum_);

            EV << NOW << " AmTxQueue::addPdus current SDU size "
               << currentSdu_->getByteLength() << " will be fragmented in "
               << fragDesc_.totalFragments_ << " PDUs, each  of size "
               << fragDesc_.fragUnit_ << endl;

            if (lteInfo_ != NULL)
            delete lteInfo_;

            lteInfo_ = check_and_cast<FlowControlInfo*>(
                currentSdu_->getControlInfo()->dup());
        }
        // Create a copy of the SDU
        LteRlcAmSdu* sduCopy = currentSdu_->dup();
        // duplicate SDU control info
        FlowControlInfo* lteInfo = lteInfo_->dup();

        EV << NOW << " AmTxQueue::addPdus -  create a new RLC PDU" << endl;
        LteRlcAmPdu * pdu = new LteRlcAmPdu("rlcAmPdu");
        // set RLC type descriptor
        pdu->setAmType(DATA);
        // set fragmentation info
        pdu->setTotalFragments(fragDesc_.totalFragments_);
        pdu->setSnoFragment(txWindowDesc_.seqNum_);
        pdu->setFirstSn(fragDesc_.firstSn_);
        pdu->setLastSn(fragDesc_.firstSn_ + fragDesc_.totalFragments_ - 1);
        pdu->setSnoMainPacket(sduCopy->getSnoMainPacket());
        // encapsulate main SDU
        pdu->encapsulate(sduCopy);
        // set fragment size
        pdu->setByteLength(fragDesc_.fragUnit_);
        // set control info
        pdu->setControlInfo(lteInfo);
        // set this as the first transmission for PDU
        pdu->setTxNumber(0);
        // try the insertion into tx buffer
        int txWindowIndex = txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_;

        if (pduRtxQueue_.get(txWindowIndex) == NULL)
        {
            // store a copy of current PDU
            LteRlcAmPdu * pduCopy = pdu->dup();
            pduCopy->setControlInfo(lteInfo->dup());
            pduRtxQueue_.addAt(txWindowIndex, pduCopy);

            if (received_.at(txWindowIndex) || discarded_.at(txWindowIndex))
            throw cRuntimeError("AmTxQueue::addPdus(): trying to add a PDU to a  position marked received [%d] discarded [%d]",
                (int)(received_.at(txWindowIndex)) ,(int)(discarded_.at(txWindowIndex)));
        }
        else
        {
            throw cRuntimeError("AmTxQueue::addPdus(): trying to add a PDU to a busy position [%d]", txWindowIndex);
        }
        // Start the PDU timer
        pduTimer_.add(pduRtxTimeout_, txWindowDesc_.seqNum_);

        // Update number of added PDUs for the current SDU and check if all fragments have been transmitted
        if (fragDesc_.addFragment())
        {
            fragDesc_.resetFragmentation();
            currentSdu_ = NULL;
        }
        // Update Sequence Number
        txWindowDesc_.seqNum_++;
        addedPdus++;

        //activate buffer checking timer
        if (bufferStatusTimer_.busy() == false)
        {
            bufferStatusTimer_.start(bufferStatusTimeout_);
        }

        // send down the PDU
        LteRlcAm* lteRlc = check_and_cast<LteRlcAm *>(
            getParentModule()->getSubmodule("am"));
        lteRlc->sendFragmented(pdu);
    }

    EV << NOW << " AmTxQueue::addPdus - added " << addedPdus << " PDUs" << endl;
}

void AmTxQueue::discard(const int seqNum)
{
    int txWindowIndex = seqNum - txWindowDesc_.firstSeqNum_;

    EV << NOW << " AmTxQueue::discard sequence number [" << seqNum
       << "] window index [" << txWindowIndex << "]" << endl;

    if ((txWindowIndex < 0) || (txWindowIndex >= txWindowDesc_.windowSize_))
    {
        throw cRuntimeError(" AmTxQueue::discard(): requested to discard an out of window PDU :"
            " sequence number %d , window first sequence is %d",
            seqNum, txWindowDesc_.firstSeqNum_);
    }

    if (discarded_.at(txWindowIndex) == true)
    {
        EV << " AmTxQueue::discard requested to discard an already discarded  PDU :"
        " sequence number" << seqNum << " , window first sequence is " << txWindowDesc_.firstSeqNum_ << endl;
    }
    else
    {
        // mark current PDU for discard
        discarded_.at(txWindowIndex) = true;
    }

    LteRlcAmPdu* pdu = check_and_cast<LteRlcAmPdu*>(
        pduRtxQueue_.get(txWindowIndex));

    if (pduTimer_.busy(seqNum))
        pduTimer_.remove(seqNum);

    LteRlcAmPdu* nextPdu = NULL;
    // Check forward in the buffer if there are other PDUs related to the same SDU
    for (int i = (txWindowIndex + 1);
        i < (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_); ++i)
    {
        if (pduRtxQueue_.get(i) != NULL)
        {
            nextPdu = check_and_cast<LteRlcAmPdu*>(pduRtxQueue_.get(i));
            if (pdu->getSnoMainPacket() == nextPdu->getSnoMainPacket())
            {
                // Mark the PDU to be discarded
                if (!discarded_.at(i))
                {
                    discarded_.at(i) = true;
                    // Stop the timer
                    if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_))
                        pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
                }
            }
            else
            {
                // PDU belonging to different SDUs found . stopping forward search
                break;
            }
        }
        else
            break; // last PDU in bufer found , stopping forward search
    }
    // Check backward in the buffer if there are other PDUs related to the same SDU
    for (int i = txWindowIndex - 1; i >= 0; i--)
    {
        if (pduRtxQueue_.get(i) == NULL)
            throw cRuntimeError("AmTxBuffer::discard(): trying to get access to missing PDU %d", i);

        nextPdu = check_and_cast<LteRlcAmPdu*>(pduRtxQueue_.get(i));

        if (pdu->getSnoMainPacket() == nextPdu->getSnoMainPacket())
        {
            if (!discarded_.at(i))
            {
                // Mark the PDU to be discarded
                discarded_.at(i) = true;
            }
            // Stop the timer
            if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_))
                pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
        }
        else
        {
            break;
        }
    }
    // check if a move receiver window command can be issued.
    checkForMrw();
}

void AmTxQueue::checkForMrw()
{
    EV << NOW << " AmTxQueue::checkForMrw " << endl;

    // If there is a discarded RLC PDU at the beginning of the buffer, try
    // to move the transmitter window
    int lastPdu = 0;
    bool toMove = false;

    for (int i = 0; i < (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_); ++i)
    {
        if ((discarded_.at(i) == true) || (received_.at(i) == true))
        {
            lastPdu = i;
            toMove = true;
        }
        else
        {
            break;
        }
    }

    if (toMove == true)
    {
        int lastSn = txWindowDesc_.firstSeqNum_ + lastPdu;

        EV << NOW << " AmTxQueue::checkForMrw  detected a shift from " << lastSn << endl;

        sendMrw(lastSn + 1);
    }
}

void AmTxQueue::moveTxWindow(const int seqNum)
{
    int pos = seqNum - txWindowDesc_.firstSeqNum_;

    // ignore the shift, it is uneffective.
    if (pos <= 0)
        return;

    // Shift the tx window - pos is the location after the last PDU to be removed (pointing to the new firstSeqNum_).

    EV << NOW << " AmTxQueue::moveTxWindow sequence number " << seqNum
       << " corresponding index " << pos << endl;

    // Delete both discarded and received RLC PDUs
    LteRlcAmPdu* pdu = NULL;

    for (int i = 0; i < pos; ++i)
    {
        if (pduRtxQueue_.get(i) != NULL)
        {
            EV << NOW << " AmTxQueue::moveTxWindow deleting PDU ["
               << i + txWindowDesc_.firstSeqNum_
               << "] corresponding index " << i << endl;

            pdu = check_and_cast<LteRlcAmPdu*>(pduRtxQueue_.remove(i));
            delete pdu;
            // Stop the rtx timer event
            if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_))
            {
                pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
                EV << NOW << " AmTxQueue::moveTxWindow canceling PDU timer ["
                   << i + txWindowDesc_.firstSeqNum_
                   << "] corresponding index " << i << endl;
            }
            received_.at(i)=false;
            discarded_.at(i)=false;
        }
        else
        throw cRuntimeError("AmTxQueue::moveTxWindow(): encountered empty PDU at location %d, shift position %d", i, pos);
    }

    for (int i = pos;
        i < ((txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_)); ++i)
    {
        if (pduRtxQueue_.get(i) != NULL)
        {
            pdu = check_and_cast<LteRlcAmPdu*>(pduRtxQueue_.remove(i));
            pduRtxQueue_.addAt(i - pos, pdu);

            EV << NOW << " AmTxQueue::moveTxWindow  PDU ["
               << i + txWindowDesc_.firstSeqNum_
               << "] corresponding index " << i
               << " being moved at position " << i - pos << endl;
        }
        else
        {
            throw cRuntimeError("AmTxQueue::moveTxWindow(): encountered empty PDU at location %d, shift position %d", i, pos);
        }

        EV << NOW << " AmTxQueue::moveTxWindow  PDU ["
           << i + txWindowDesc_.firstSeqNum_
           << "] corresponding new index " << i - pos
           << " marked as received [" << (received_.at(i - pos))
           << "] , discarded [" << (discarded_.at(i - pos)) << "]" << endl;

        received_.at(i - pos) = received_.at(i);
        discarded_.at(i - pos) = discarded_.at(i);

        received_.at(i) = false;
        discarded_.at(i) = false;

        EV << NOW << " AmTxQueue::moveTxWindow  location [" << i
           << "] corresponding new PDU "
           << i + pos + txWindowDesc_.firstSeqNum_
           << " marked as received [" << (received_.at(i))
           << "] , discarded [" << (discarded_.at(i)) << "]" << endl;
    }

            // cleanup
    for (int i = (txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_);
        i < txWindowDesc_.windowSize_; ++i)
    {
        if (pduRtxQueue_.get(i) != NULL)
            throw cRuntimeError("AmTxQueue::moveTxWindow(): encountered busy PDU at location %d, shift position %d", i,
                pos);

        EV << NOW << " AmTxQueue::moveTxWindow  empty location [" << i
           << "] marked as received [" << (received_.at(i))
           << "] , discarded [" << (discarded_.at(i)) << "]" << endl;

        received_.at(i) = false;
        discarded_.at(i) = false;
    }

    txWindowDesc_.firstSeqNum_ += pos;

    EV << NOW << " AmTxQueue::moveTxWindow completed. First sequence number "
       << txWindowDesc_.firstSeqNum_ << " current sequence number "
       << txWindowDesc_.seqNum_ << " cleaned up locations from "
       << txWindowDesc_.seqNum_ - txWindowDesc_.firstSeqNum_ << " to "
       << txWindowDesc_.windowSize_ - 1 << endl;

    // Try to add more PDUs to the buffer
    addPdus();
}

void AmTxQueue::sendMrw(const int seqNum)
{
    EV << NOW << " AmTxQueue::sendMrw sending MRW PDU [" << mrwDesc_.mrwSeqNum_
       << "]for sequence number " << seqNum << endl;

    // duplicate SDU control info
    FlowControlInfo* lteInfo = lteInfo_->dup();

    // create a new RLC PDU
    LteRlcAmPdu * pdu = new LteRlcAmPdu("rlcAmPdu");
    // set RLC type descriptor
    pdu->setAmType(MRW);
    // set fragmentation info
    pdu->setTotalFragments(1);
    pdu->setSnoFragment(mrwDesc_.mrwSeqNum_);
    pdu->setSnoMainPacket(mrwDesc_.mrwSeqNum_);
    // set fragment size
    pdu->setByteLength(fragDesc_.fragUnit_);
    // prepare MRW control data
    pdu->setFirstSn(0);
    pdu->setLastSn(seqNum);
    // set control info
    pdu->setControlInfo(lteInfo);

    LteRlcAmPdu * pduCopy = pdu->dup();
    pduCopy->setControlInfo(lteInfo->dup());
    //  save copy for retransmission
    mrwRtxQueue_.addAt(mrwDesc_.mrwSeqNum_, pduCopy);
    // update MRW descriptor
    mrwDesc_.lastMrw_ = mrwDesc_.mrwSeqNum_;
    // Start a timer for MRW message
    mrwTimer_.add(ctrlPduRtxTimeout_, mrwDesc_.mrwSeqNum_);
    // Increment mrwSn_
    mrwDesc_.mrwSeqNum_++;
    // Send the MRW message
    sendPdu(pdu);
}

void AmTxQueue::sendPdu(LteRlcAmPdu* pdu)
{
    Enter_Method("sendCtrlPdu()");
    // Pass the RLC PDU to the MAC
    LteRlcAm* lteRlc = check_and_cast<LteRlcAm *>(
        getParentModule()->getSubmodule("am"));
    lteRlc->sendFragmented(pdu);
}

void AmTxQueue::handleControlPacket(cPacket* pkt)
{
    Enter_Method("handleControlPacket()");
    LteRlcAmPdu * pdu = check_and_cast<LteRlcAmPdu*>(pkt);
    // get RLC type descriptor
    short type = pdu->getAmType();

    switch (type)
    {
        case MRW_ACK:
            EV << NOW << " AmTxQueue::handleControlPacket , received MRW ACK ["
               << pdu->getSnoMainPacket() << "]: window new first SN  "
               << pdu->getSnoFragment() << endl;
            // move tx window
            moveTxWindow(pdu->getSnoFragment());
            // signal ACK reception
            recvMrwAck(pdu->getSnoMainPacket());
            break;

            case ACK:
            EV << NOW << " AmTxQueue::handleControlPacket , received ACK " << endl;
            recvCumulativeAck(pdu->getLastSn());

            int bSize = pdu->getBitmapArraySize();

            if (bSize > 0)
            {
                EV << NOW
                   << " AmTxQueue::handleControlPacket , received BITMAP ACK of size "
                   << bSize << endl;

                for (int i = 0; i < bSize; ++i)
                {
                    if (pdu->getBitmap(i))
                    {
                        recvAck(pdu->getFirstSn() + i);
                    }
                }
            }

            break;
        }

        ASSERT(pdu->getOwner() == this);
        delete pdu;
    }

void AmTxQueue::recvAck(const int seqNum)
{
    int index = seqNum - txWindowDesc_.firstSeqNum_;

    EV << NOW << " AmTxBuffer::recvAck ACK received for sequence number "
       << seqNum << " first sequence n. [" << txWindowDesc_.firstSeqNum_
       << "] index [" << index << "] " << endl;

    if (index < 0)
    {
        EV << NOW
           << " AmTxBuffer::recvAck ACK already received - ignoring : index "
           << index << " first sequence number"
           << txWindowDesc_.firstSeqNum_ << endl;

        return;
    }

    if (index >= txWindowDesc_.windowSize_)
        throw cRuntimeError("AmTxBuffer::recvAck(): ACK greater than window size %d", txWindowDesc_.windowSize_);

    if (!(received_.at(index)))
    {
        EV << NOW << " AmTxBuffer::recvAck canceling timer for PDU "
           << (index + txWindowDesc_.firstSeqNum_) << " index " << index << endl;
        // Stop the timer
        if (pduTimer_.busy(index + txWindowDesc_.firstSeqNum_))
        pduTimer_.remove(index + txWindowDesc_.firstSeqNum_);
        // Received status variable is set at true after the
        received_.at(index) = true;
    }
}

void AmTxQueue::recvCumulativeAck(const int seqNum)
{
    // Mark the AM PDUs as received and shift the window
    if ((seqNum < txWindowDesc_.firstSeqNum_) || (seqNum < 0))
    {
        // Ignore the cumulative ACK, is out of the transmitter window (the MRW command has not yet been received by AM rx entity)
        return;
    }
    else if ((unsigned int) seqNum
        > (txWindowDesc_.firstSeqNum_ + txWindowDesc_.windowSize_))
    {
        throw cRuntimeError("AmTxQueue::recvCumulativeAck(): SN %d exceeds window size %d",
            seqNum, txWindowDesc_.windowSize_);
    }
    else
    {
        // The ACK is inside the window

        for (int i = 0; i <= (seqNum - txWindowDesc_.firstSeqNum_); ++i)
        {
            EV << NOW
               << " AmTxBuffer::recvCumulativeAck ACK received for sequence number "
               << (i + txWindowDesc_.firstSeqNum_)
               << " first sequence n. [" << txWindowDesc_.firstSeqNum_
               << "] "
            "index [" << i << "] " << endl;

            // the ACK could have already been received
            if (!(received_.at(i)))
            {
                // canceling timer for PDU
                EV << NOW
                   << " AmTxBuffer::recvCumulativeAck canceling timer for PDU "
                   << (i + txWindowDesc_.firstSeqNum_) << " index " << i << endl;
                // Stop the timer
                if (pduTimer_.busy(i + txWindowDesc_.firstSeqNum_))
                pduTimer_.remove(i + txWindowDesc_.firstSeqNum_);
                // Received status variable is set at true after the
                received_.at(i) = true;
            }
        }
        checkForMrw();
    }
}

void AmTxQueue::recvMrwAck(const int seqNum)
{
    EV << NOW << " AmTxQueue::recvMrwAck for MRW command number " << seqNum << endl;

    if (mrwRtxQueue_.get(seqNum) == NULL)
    {
        // The message is related to a MRW which has been discarded by the handle function because it was obsolete.
        return;
    }

    // Remove the MRW PDU from the retransmission buffer
    LteRlcAmPdu* mrwPdu = check_and_cast<LteRlcAmPdu*>(
        mrwRtxQueue_.remove(seqNum));

    // Stop the related timer
    if (mrwTimer_.busy(seqNum))
    {
        mrwTimer_.remove(seqNum);
    }
    //deallocate the MRW PDU
    delete mrwPdu;
}

void AmTxQueue::pduTimerHandle(const int sn)
{
    Enter_Method("pduTimerHandle");
    // A timer is elapsed for the RLC PDU.
    // This function checks if the PDU has been correctly received.
    // If not, the handle checks if another transmission is possible, and
    // in this case the PDU is placed in the retransmission buffer.
    // If no more attempt could be done the PDU is dropped.

    int index = sn - txWindowDesc_.firstSeqNum_;

    EV << NOW << " AmTxQueue::pduTimerHandle - sequence number " << sn << endl;

    // signal event handling to timer
    pduTimer_.handle(sn);

    // Some debug checks
    if ((index < 0) || (index >= txWindowDesc_.windowSize_))
        throw cRuntimeError(
            "AmTxQueue::pduTimerHandle(): The PDU [%d] for which timer elapsed is out of the window : index [%d]", sn,
            index);

    if (pduRtxQueue_.get(index) == NULL)
        throw cRuntimeError("AmTxQueue::pduTimerHandle(): PDU %d not found", index);

    // Check if the PDU has been correctly received, if so the
    // timer should have been previously stopped.
    if (received_.at(index) == true)
        throw cRuntimeError(" AmTxQueue::pduTimerHandle(): The PDU %d [index %d] has been already received", sn, index);

    // Get the PDU information
    LteRlcAmPdu* pdu = check_and_cast<LteRlcAmPdu*>(pduRtxQueue_.get(index));

    int nextTxNumber = pdu->getTxNumber() + 1;

    if (nextTxNumber > maxRtx_)
    {
        EV << NOW << " AmTxQueue::pduTimerHandle maximum transmission reached, discard the PDU" << endl;
        // The maximum number of transmission for this PDU has been
        // reached. Discard the PDU and all the PDUs related to the
        // same RLC SDU.
        discard(sn);
    }
    else
    {
        EV << NOW << " AmTxQueue::pduTimerHandle starting new transmission" << endl;
        // extract PDU from buffer
        pdu = check_and_cast<LteRlcAmPdu*>(pduRtxQueue_.remove(index));
        // A new transmission can be started
        pdu->setTxNumber(nextTxNumber);
        // The RLC PDU is added to the retransmission buffer
        LteRlcAmPdu* copy = pdu->dup();
        // .. with control info also!
        copy->setControlInfo(pdu->getControlInfo()->dup());

        pduRtxQueue_.add(copy);
        // Reschedule the timer
        pduTimer_.add(pduRtxTimeout_, sn);
        // send down the PDU
        sendPdu(pdu);
    }
}

void AmTxQueue::mrwTimerHandle(const int sn)
{
    EV << NOW << " AmTxQueue::mrwTimerHandle MRW_ACK sn: " << sn << endl;

    mrwTimer_.handle(sn);

    if (mrwRtxQueue_.get(sn) == NULL)
        throw cRuntimeError("MRW handler: MRW of SN %d not found in MRW message queue", sn);

    // Check if a newer message has been sent
    if (mrwDesc_.lastMrw_ > sn)
    {
        EV << NOW << "AmTxBuffer::mrwTimerHandle newer MRW has been sent - no action has to be taken" << endl;

        // A newer message has been sent
        // Delete the RLC  PDU
        delete (mrwRtxQueue_.remove(sn));
    }
    else
    {
        EV << NOW << "AmTxBuffer::mrwTimerHandle retransmitting MRW" << endl;

        LteRlcAmPdu* pdu = check_and_cast<LteRlcAmPdu*>(
            mrwRtxQueue_.remove(sn));
        // Retransmit the MRW message
        LteRlcAmPdu* pduCopy = pdu->dup();
        pduCopy->setControlInfo(pdu->getControlInfo()->dup());
        // Enqueue the PDU into the retransmission buffer
        mrwRtxQueue_.addAt(sn, pduCopy);
        // Retransmit the MRW control message
        mrwTimer_.add(ctrlPduRtxTimeout_, sn);
        sendPdu(pdu);
    }
}

void AmTxQueue::handleMessage(cMessage* msg)
{
    if (msg->isName("timer"))
    {
        // message received from a timer
        TTimerMsg* tmsg = check_and_cast<TTimerMsg *>(msg);
        // check timer id
        RlcAmTimerType amType = static_cast<RlcAmTimerType>(tmsg->getTimerId());
        TTimerType type = static_cast<TTimerType>(tmsg->getType());

        switch (type)
        {
            case TTSIMPLE:
                // Check the buffer status and eventually send an MRW command.
                checkForMrw();
                // signal the timer the event has been handled
                bufferStatusTimer_.handle();
                break;
            case TTMULTI:

                TMultiTimerMsg* tmtmsg = check_and_cast<TMultiTimerMsg*>(tmsg);

                if (amType == PDU_T)
                {
                    pduTimerHandle(tmtmsg->getEvent());
                }
                else if (amType == MRW_T)
                {
                    mrwTimerHandle(tmtmsg->getEvent());
                }
                else
                    throw cRuntimeError("AmTxQueue::handleMessage(): unexpected timer event received");
                break;
        }
        // delete timer event message.
        delete tmsg;
    }
    return;
}
