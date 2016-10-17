//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "UmTxEntity.h"

Define_Module(UmTxEntity);

/*
 * Main functions
 */

void UmTxEntity::initialize()
{
    sno_ = 0;
    firstIsFragment_ = false;

    // store the node id of the owner module
    LteMacBase* mac = check_and_cast<LteMacBase*>(getParentModule()->getParentModule()->getSubmodule("mac"));
    ownerNodeId_ = mac->getMacNodeId();
}

void UmTxEntity::enque(cPacket* pkt)
{
    EV << NOW << " UmTxEntity::enque - bufferize new SDU  " << endl;
    // Buffer the SDU
    sduQueue_.insert(pkt);
}

void UmTxEntity::rlcPduMake(int pduLength)
{
    EV << NOW << " UmTxEntity::rlcPduMake - PDU with size " << pduLength << " requested from MAC"<< endl;

    // create the RLC PDU
    LteRlcUmDataPdu* rlcPdu = new LteRlcUmDataPdu("lteRlcFragment");

    // the request from MAC takes into account also the size of the RLC header
    pduLength -= RLC_HEADER_UM;

    int len = 0;

    bool startFrag = firstIsFragment_;
    bool endFrag = false;

    while (!sduQueue_.empty() && pduLength > 0)
    {
        // detach data from the SDU buffer
        cPacket* pkt = sduQueue_.front();
        LteRlcSdu* rlcSdu = check_and_cast<LteRlcSdu*>(pkt);

        unsigned int sduSequenceNumber = rlcSdu->getSnoMainPacket();
        int sduLength = rlcSdu->getByteLength();

        EV << NOW << " UmTxEntity::rlcPduMake - Next data chunk from the queue, sduSno[" << sduSequenceNumber
                << "], length[" << sduLength << "]"<< endl;

        if (pduLength >= sduLength)
        {
            EV << NOW << " UmTxEntity::rlcPduMake - Add " << sduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

            // add the whole SDU
            pduLength -= sduLength;
            len += sduLength;

            sduQueue_.pop();

            rlcPdu->pushSdu(rlcSdu);

            EV << NOW << " UmTxEntity::rlcPduMake - Pop data chunk from the queue, sduSno[" << sduSequenceNumber << "]" << endl;

            // now, the first SDU is a fragment
            firstIsFragment_ = false;

            EV << NOW << " UmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;
        }
        else
        {
            EV << NOW << " UmTxEntity::rlcPduMake - Add " << pduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

            // add partial SDU

            len += pduLength;

            LteRlcSdu* rlcSduDup = rlcSdu->dup();
            rlcSduDup->setByteLength(pduLength);
            rlcPdu->pushSdu(rlcSduDup);

            endFrag = true;

            // update SDU in the buffer
            int newLength = sduLength - pduLength;
            pkt->setByteLength(newLength);

            EV << NOW << " UmTxEntity::rlcPduMake - Data chunk in the queue is now " << newLength << " bytes, sduSno[" << sduSequenceNumber << "]" << endl;

            pduLength = 0;

            // now, the first SDU in the buffer is a fragment
            firstIsFragment_ = true;

            EV << NOW << " UmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;

        }
    }

    if (len == 0)
    {
        // send an empty message to notify the MAC that there is not enough space to send RLC PDU
        rlcPdu->setControlInfo(flowControlInfo_->dup());
        rlcPdu->setByteLength(len);
    }
    else
    {
        // compute FI
        // the meaning of this field is specified in 3GPP TS 36.322
        FramingInfo fi = 0;
        unsigned short int mask;
        if (endFrag)
        {
            mask = 1;   // 01
            fi |= mask;
        }
        if (startFrag)
        {
            mask = 2;   // 10
            fi |= mask;
        }

        rlcPdu->setFramingInfo(fi);
        rlcPdu->setPduSequenceNumber(sno_++);
        rlcPdu->setControlInfo(flowControlInfo_->dup());
        rlcPdu->setByteLength(RLC_HEADER_UM + len);  // add the header size
    }

    // send to MAC layer
    EV << NOW << " UmTxEntity::rlcPduMake - send PDU " << rlcPdu->getPduSequenceNumber() << " with size " << rlcPdu->getByteLength() << " bytes to lower layer" << endl;

    LteRlcUm* lteRlc = check_and_cast<LteRlcUm *>(getParentModule()->getSubmodule("um"));
    lteRlc->sendToLowerLayer(rlcPdu);
}

void UmTxEntity::removeDataFromQueue()
{
    EV << NOW << " UmTxEntity::removeDataFromQueue - removed SDU " << endl;

    // get the last packet...
    cPacket* pkt = sduQueue_.back();

    // ...and remove it
    cPacket* retPkt = sduQueue_.remove(pkt);
    delete retPkt;
}

void UmTxEntity::rlcHandleD2DModeSwitch(bool oldConnection)
{
    if (oldConnection)
    {
        if (getNodeTypeById(ownerNodeId_) == ENODEB)
        {
            EV << NOW << " UmRxEntity::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
            return;
        }

        EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - clear TX buffer of the RLC entity associated to the old mode" << endl;

        // empty buffer
        while (!sduQueue_.isEmpty())
            delete sduQueue_.pop();

        // reset variables except for sequence number
        firstIsFragment_ = false;
    }
    else
    {
        EV << " UmTxEntity::rlcHandleD2DModeSwitch - reset numbering of the RLC TX entity corresponding to the new mode" << endl;
        sno_ = 0;
    }
}
