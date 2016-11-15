//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteHarqBufferRx.h"
#include "LteMacPdu.h"
#include "LteControlInfo.h"
#include "LteHarqFeedback_m.h"
#include "LteMacBase.h"
#include "LteMacEnb.h"

LteHarqBufferRx::LteHarqBufferRx(unsigned int num, LteMacBase *owner,
    MacNodeId nodeId)
{
    macOwner_ = owner;
    nodeId_ = nodeId;
    numHarqProcesses_ = num;
    processes_.resize(numHarqProcesses_);

    for (unsigned int i = 0; i < numHarqProcesses_; i++)
    {
        processes_[i] = new LteHarqProcessRx(i, macOwner_);
    }
    tSampleCell_ = new TaggedSample();
    tSample_ = new TaggedSample();
    /* Signals initialization: those are used to gather statistics */

    if (macOwner_->getNodeType() == ENODEB)
    {
        nodeB_ = macOwner_;
        macDelay_ = macOwner_->registerSignal("macDelayUl");
        macThroughput_ = getMacByMacNodeId(nodeId_)->registerSignal("macThroughputUl");
        macCellThroughput_ = macOwner_->registerSignal("macCellThroughputUl");

        tSampleCell_->module_ = check_and_cast<cComponent*>(macOwner_);
        tSample_->module_ = check_and_cast<cComponent*>(getMacByMacNodeId(nodeId_));
    }
    else if (macOwner_->getNodeType() == UE)
    {
        nodeB_ = getMacByMacNodeId(macOwner_->getMacCellId());
        macThroughput_ = macOwner_->registerSignal("macThroughputDl");
        macCellThroughput_ = nodeB_->registerSignal("macCellThroughputDl");
        macDelay_ = macOwner_->registerSignal("macDelayDl");

        tSampleCell_->module_ = nodeB_;
        tSample_->module_ = macOwner_;
    }
}

void LteHarqBufferRx::insertPdu(Codeword cw, LteMacPdu *pdu)
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
    EV << "H-ARQ RX: new pdu (id " << pdu->getId()
       << " ) inserted into process " << (int) acid << endl;
}

void LteHarqBufferRx::sendFeedback()
{
    for (unsigned int i = 0; i < numHarqProcesses_; i++)
    {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw)
        {
            if (processes_[i]->isEvaluated(cw))
            {
                LteHarqFeedback *hfb = processes_[i]->createFeedback(cw);

                // debug output:
                const char *r = hfb->getResult() ? "ACK" : "NACK";
                EV << "H-ARQ RX: feedback sent to TX process "
                   << (int) hfb->getAcid() << " Codeword  " << (int) cw
                   << "of node with id "
                   << check_and_cast<UserControlInfo *>(
                    hfb->getControlInfo())->getDestId()
                   << " result: " << r << endl;

                macOwner_->takeObj(hfb);
                macOwner_->sendLowerPackets(hfb);
            }
        }
    }
}

unsigned int LteHarqBufferRx::purgeCorruptedPdus()
{
    unsigned int purged = 0;

    for (unsigned int i = 0; i < numHarqProcesses_; i++)
    {
        for (Codeword cw = 0; cw < MAX_CODEWORDS; ++cw)
        {
            if (processes_[i]->getUnitStatus(cw) == RXHARQ_PDU_CORRUPTED)
            {
                EV << "LteHarqBufferRx::purgeCorruptedPdus - purged pdu with acid " << i << endl;
                // purge PDU
                processes_[i]->purgeCorruptedPdu(cw);
                processes_[i]->resetCodeword(cw);
                //increment purged PDUs counter
                ++purged;
            }
        }
    }
    return purged;
}

std::list<LteMacPdu *> LteHarqBufferRx::extractCorrectPdus()
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
                UserControlInfo* info = check_and_cast<UserControlInfo*>(
                    temp->getControlInfo());

                // Calculate delay by subtracting the arrival time
                // to the MAC packet creation time
                tSample_->sample_ = (NOW - temp->getCreationTime()).dbl();
                if (info->getDirection() == DL)
                {
                    tSample_->id_ = info->getDestId();
                }
                else if (info->getDirection() == UL)
                {
                    tSample_->id_ = info->getSourceId();
                }
                else
                {
                    throw cRuntimeError("LteHarqBufferRx::extractCorrectPdus(): unknown direction %d",(int) info->getDirection());
                }

                // emit delay statistic
                macOwner_->emit(macDelay_, tSample_);

                // Calculate Throughput by sending the number of bits for this packet
                tSample_->sample_ = size;
                tSampleCell_->sample_ = size;
                if (macOwner_->getNodeType() == UE)
                {
                    tSample_->id_ = info->getDestId();
                    tSampleCell_->id_ = macOwner_->getMacCellId();
                }
                else if (macOwner_->getNodeType() == ENODEB)
                {
                    tSample_->id_ = info->getSourceId();
                    tSampleCell_->id_ = info->getDestId();
                }
                else
                {
                    throw cRuntimeError("LteHarqBufferRx::extractCorrectPdus(): unknown nodeType %d",
                        (int) macOwner_->getNodeType());
                }

                // emit throughput statistics
                nodeB_->emit(macCellThroughput_, tSampleCell_);
                macOwner_->emit(macThroughput_, tSample_);

                macOwner_->dropObj(temp);
                ret.push_back(temp);
                acid = i;

                EV << "LteHarqBufferRx::extractCorrectPdus H-ARQ RX: pdu (id " << ret.back()->getId()
                   << " ) extracted from process " << (int) acid
                   << "to be sent upper" << endl;
            }
        }
    }

    return ret;
}

RxBufferStatus LteHarqBufferRx::getBufferStatus()
{
    RxBufferStatus bs(numHarqProcesses_);
    unsigned int numHarqUnits = 0;
    for (unsigned int i = 0; i < numHarqProcesses_; i++)
    {
        numHarqUnits = (processes_)[i]->getNumHarqUnits();
        std::vector<RxUnitStatus> vus(numHarqUnits);
        vus = (processes_)[i]->getProcessStatus();
        bs[i] = vus;
    }
    return bs;
}

LteHarqBufferRx::~LteHarqBufferRx()
{
    std::vector<LteHarqProcessRx *>::iterator it = processes_.begin();
    for (; it != processes_.end(); ++it)
        delete *it;
    processes_.clear();
    macOwner_ = NULL;

    delete tSampleCell_;
    delete tSample_;
}
