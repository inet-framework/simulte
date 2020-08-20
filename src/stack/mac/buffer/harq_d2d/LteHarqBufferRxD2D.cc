//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "common/LteControlInfo.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/layer/LteMacBase.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/layer/LteMacUeD2D.h"
#include "stack/mac/layer/LteMacEnbD2D.h"

unsigned int LteHarqBufferRxD2D::totalCellRcvdBytesD2D_ = 0;

LteHarqBufferRxD2D::LteHarqBufferRxD2D(unsigned int num, LteMacBase *owner, MacNodeId nodeId, bool isMulticast)
{
    macOwner_ = owner;
    nodeId_ = nodeId;
    macUe_ = check_and_cast<LteMacBase*>(getMacByMacNodeId(nodeId_));
    numHarqProcesses_ = num;
    processes_.resize(numHarqProcesses_);
    totalRcvdBytes_ = 0;
    isMulticast_ = isMulticast;

    for (unsigned int i = 0; i < numHarqProcesses_; i++)
    {
        processes_[i] = new LteHarqProcessRxD2D(i, macOwner_);
    }

    /* Signals initialization: those are used to gather statistics */

    if (macOwner_->getNodeType() == ENODEB)
    {
        nodeB_ = macOwner_;
        macDelay_ = macOwner_->registerSignal("macDelayUl");
        macThroughput_ = getMacByMacNodeId(nodeId_)->registerSignal("macThroughputUl");
        macCellThroughput_ = macOwner_->registerSignal("macCellThroughputUl");
    }
    else if (macOwner_->getNodeType() == UE)
    {
        nodeB_ = getMacByMacNodeId(macOwner_->getMacCellId());
        macThroughput_ = macOwner_->registerSignal("macThroughputDl");
        macCellThroughput_ = nodeB_->registerSignal("macCellThroughputDl");
        macDelay_ = macOwner_->registerSignal("macDelayDl");

        // if D2D is enabled, register also D2D statistics
        if (macOwner_->isD2DCapable())
        {
            macThroughputD2D_ = check_and_cast<LteMacUeD2D*>(macOwner_)->registerSignal("macThroughputD2D");
            macDelayD2D_ = check_and_cast<LteMacUeD2D*>(macOwner_)->registerSignal("macDelayD2D");
            macCellThroughputD2D_ = check_and_cast<LteMacEnbD2D*>(nodeB_)->registerSignal("macCellThroughputD2D");
        }
    }
}

void LteHarqBufferRxD2D::insertPdu(Codeword cw, LteMacPdu *pdu)
{
    UserControlInfo *uInfo = check_and_cast<UserControlInfo *>(pdu->getControlInfo());

    MacNodeId srcId = uInfo->getSourceId();
    if (macOwner_->isHarqReset(srcId))
    {
        // if the HARQ processes have been aborted during this TTI (e.g. due to a D2D mode switch),
        // incoming packets should not be accepted
        delete pdu;
        return;
    }

    unsigned char acid = uInfo->getAcid();
    // TODO add codeword to inserPdu
    processes_[acid]->insertPdu(cw, pdu);
    // debug output
    EV << "H-ARQ RX: new pdu (id " << pdu->getId() << " ) inserted into process " << (int) acid << endl;
}

void LteHarqBufferRxD2D::sendFeedback()
{
    for (unsigned int i = 0; i < numHarqProcesses_; i++)
    {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw)
        {
            if (processes_[i]->isEvaluated(cw))
            {
                // create a copy of the feedback to be sent to the eNB
                LteHarqFeedbackMirror *mirrorHfb = check_and_cast<LteHarqProcessRxD2D*>(processes_[i])->createFeedbackMirror(cw);
                if (mirrorHfb == NULL)
                {
                    EV<<NOW<<"LteHarqBufferRxD2D::sendFeedback - cw "<< cw << " of process " << i
                            << " contains a pdu belonging to a multicast/broadcast connection. Don't send feedback mirror." << endl;
                }
                else
                {
                    macOwner_->sendLowerPackets(mirrorHfb);
                }

                LteHarqFeedback *hfb = processes_[i]->createFeedback(cw);
                if (hfb == NULL)
                {
                    EV<<NOW<<"LteHarqBufferRxD2D::sendFeedback - cw "<< cw << " of process " << i
                            << " contains a pdu belonging to a multicast/broadcast connection. Don't send feedback." << endl;
                    continue;
                }

                // debug output:
                const char *r = hfb->getResult() ? "ACK" : "NACK";
                EV << "H-ARQ RX: feedback sent to TX process "
                   << (int) hfb->getAcid() << " Codeword  " << (int) cw
                   << "of node with id "
                   << check_and_cast<UserControlInfo *>(
                    hfb->getControlInfo())->getDestId()
                   << " result: " << r << endl;

                macOwner_->sendLowerPackets(hfb);
            }
        }
    }
}

std::list<LteMacPdu *> LteHarqBufferRxD2D::extractCorrectPdus()
{
    this->sendFeedback();
    std::list<LteMacPdu*> ret;
    unsigned char acid = 0;
    for (unsigned int i = 0; i < numHarqProcesses_; i++)
    {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw)
        {
            if (processes_[i]->isCorrect(cw))
            {
                LteMacPdu* temp = processes_[i]->extractPdu(cw);
                unsigned int size = temp->getByteLength();
                UserControlInfo* info = check_and_cast<UserControlInfo*>(temp->getControlInfo());

                // emit delay statistic
                if (info->getDirection() == D2D)
                {
                    check_and_cast<LteMacUeD2D*>(macOwner_)->emit(macDelayD2D_, (NOW - temp->getCreationTime()).dbl());
                }
                else
                {
                    macUe_->emit(macDelay_, (NOW - temp->getCreationTime()).dbl());
                }

                // Calculate Throughput by sending the number of bits for this packet
                if (NOW > getSimulation()->getWarmupPeriod())
                {
                    totalRcvdBytes_ += size;
                    intervalRcvdBytes_ += size;

                    double tputSample = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

                    // emit throughput statistics
                    if (info->getDirection() == D2D)
                    {
                        totalCellRcvdBytesD2D_ += size;
                        double cellTputSample = (double)totalCellRcvdBytesD2D_ / (NOW - getSimulation()->getWarmupPeriod());
                        check_and_cast<LteMacEnbD2D*>(nodeB_)->emit(macCellThroughputD2D_, cellTputSample);
                        check_and_cast<LteMacUeD2D*>(macOwner_)->emit(macThroughputD2D_, tputSample);
                        check_and_cast<LteMacUeD2D*>(macOwner_)->emit(macPacketSize_, size);
                    }
                    else
                    {
                        if (info->getDirection() == DL)
                        {
                            totalCellRcvdBytesDl_ += size;
                            double cellTputSample = (double)totalCellRcvdBytesDl_ / (NOW - getSimulation()->getWarmupPeriod());
                            nodeB_->emit(macCellThroughput_, cellTputSample);
                            macOwner_->emit(macThroughput_, tputSample);
                            macOwner_->emit(macPacketSize_, size);
                        }
                        else  // UL
                        {
                            totalCellRcvdBytesUl_ += size;
                            double cellTputSample = (double)totalCellRcvdBytesUl_ / (NOW - getSimulation()->getWarmupPeriod());
                            nodeB_->emit(macCellThroughput_, cellTputSample);
                            macUe_->emit(macThroughput_, tputSample);
                            macUe_->emit(macPacketSize_, size);
                        }
                    }
                }

                if (NOW > getSimulation()->getWarmupPeriod() && NOW > tpIntervalStart + tpIntervalLength_)
                {
                    double tputSample = (double)intervalRcvdBytes_ / (NOW - tpIntervalStart);

                    // emit throughput statistics
                    if (info->getDirection() == D2D)
                    {
                        double cellTputSample = (double) (totalCellRcvdBytesD2D_ - intervalStartCellRcvdBytesD2D_) / (NOW - tpIntervalStart);
                        check_and_cast<LteMacEnbD2D*>(nodeB_)->emit(macCellThroughputD2D_, cellTputSample);
                        check_and_cast<LteMacUeD2D*>(macOwner_)->emit(macThroughputD2D_, tputSample);
                    }
                    else
                    {
                        if (info->getDirection() == DL)
                        {
                            double cellTputSample = (double) (totalCellRcvdBytesDl_ - intervalStartCellRcvdBytesDl_) / (NOW - tpIntervalStart);
                            nodeB_->emit(macCellThroughputInterval_, cellTputSample);
                            macOwner_->emit(macThroughputInterval_, tputSample);
                        }
                        else  // UL
                        {
                            double cellTputSample = (double) (totalCellRcvdBytesUl_ - intervalStartCellRcvdBytesUl_) / (NOW - tpIntervalStart);
                            nodeB_->emit(macCellThroughputInterval_, cellTputSample);
                            macUe_->emit(macThroughputInterval_, tputSample);
                        }
                    }
                    intervalStartCellRcvdBytesD2D_ = totalCellRcvdBytesD2D_;
                    intervalStartCellRcvdBytesDl_ = totalCellRcvdBytesDl_;
                    intervalStartCellRcvdBytesUl_ = totalCellRcvdBytesUl_;
                    intervalRcvdBytes_ = 0;
                    tpIntervalStart = NOW;
                }

                ret.push_back(temp);
                acid = i;

                EV << "LteHarqBufferRxD2D::extractCorrectPdus H-ARQ RX: pdu (id " << ret.back()->getId()
                   << " ) extracted from process " << (int) acid
                   << "to be sent upper" << endl;
            }
        }
    }

    return ret;
}

LteHarqBufferRxD2D::~LteHarqBufferRxD2D()
{
}
