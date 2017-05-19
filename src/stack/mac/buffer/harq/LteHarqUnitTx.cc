//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/harq/LteHarqUnitTx.h"
#include "stack/mac/layer/LteMacEnb.h"
#include <omnetpp.h>

LteHarqUnitTx::LteHarqUnitTx(unsigned char acid, Codeword cw,
    LteMacBase *macOwner, LteMacBase *dstMac)
{
    pdu_ = NULL;
    pduId_ = -1;
    acid_ = acid;
    cw_ = cw;
    transmissions_ = 0;
    txTime_ = 0;
    status_ = TXHARQ_PDU_EMPTY;
    macOwner_ = macOwner;
    dstMac_ = dstMac;
    maxHarqRtx_ = macOwner->par("maxHarqRtx");

    if (macOwner_->getNodeType() == ENODEB)
    {
        nodeB_ = macOwner_;
        macPacketLoss_ = dstMac_->registerSignal("macPacketLossDl");
        macCellPacketLoss_ = macOwner_->registerSignal("macCellPacketLossDl");
        harqErrorRate_ = dstMac_->registerSignal("harqErrorRateDl");
        harqErrorRate_1_ = dstMac_->registerSignal("harqErrorRate_1st_Dl");
        harqErrorRate_2_ = dstMac_->registerSignal("harqErrorRate_2nd_Dl");
        harqErrorRate_3_ = dstMac_->registerSignal("harqErrorRate_3rd_Dl");
        harqErrorRate_4_ = dstMac_->registerSignal("harqErrorRate_4th_Dl");
    }
    else  // UE
    {
        nodeB_ = getMacByMacNodeId(macOwner_->getMacCellId());
        if (dstMac_ == nodeB_)  // UL
        {
            macPacketLoss_ = macOwner_->registerSignal("macPacketLossUl");
            macCellPacketLoss_ = nodeB_->registerSignal("macCellPacketLossUl");
            harqErrorRate_ = macOwner_->registerSignal("harqErrorRateUl");
            harqErrorRate_1_ = macOwner_->registerSignal("harqErrorRate_1st_Ul");
            harqErrorRate_2_ = macOwner_->registerSignal("harqErrorRate_2nd_Ul");
            harqErrorRate_3_ = macOwner_->registerSignal("harqErrorRate_3rd_Ul");
            harqErrorRate_4_ = macOwner_->registerSignal("harqErrorRate_4th_Ul");
        }
    }
}

void LteHarqUnitTx::insertPdu(LteMacPdu *pdu)
{
    if (!pdu)
        throw cRuntimeError("Trying to insert NULL macPdu pointer in harq unit");

    // cannot insert if the unit is not idle
    if (!this->isEmpty())
        throw cRuntimeError("Trying to insert macPdu in already busy harq unit");

    pdu_ = pdu;
    pduId_ = pdu->getId();
    // as unique MacPDUId the OMNET id is used (need separate member since it must not be changed by dup())
    pdu->setMacPduId(pdu->getId());
    transmissions_ = 0;
    status_ = TXHARQ_PDU_SELECTED;
    pduLength_ = pdu_->getByteLength();
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(
        pdu_->getControlInfo());
    lteInfo->setAcid(acid_);
    Codeword cw_old = lteInfo->getCw();
    if (cw_ != cw_old)
        throw cRuntimeError("mismatch in cw settings");

    lteInfo->setCw(cw_);
}

void LteHarqUnitTx::markSelected()
{
    EV << NOW << " LteHarqUnitTx::markSelected trying to select buffer "
       << acid_ << " codeword " << cw_ << " for transmission " << endl;

    if (!(this->isReady()))
    throw cRuntimeError("ERROR acid %d codeword %d trying to select for transmission an empty buffer", acid_, cw_);

    status_ = TXHARQ_PDU_SELECTED;
}

LteMacPdu *LteHarqUnitTx::extractPdu()
{
    if (!(status_ == TXHARQ_PDU_SELECTED))
        throw cRuntimeError("Trying to extract macPdu from not selected H-ARQ unit");

    txTime_ = NOW;
    transmissions_++;
    status_ = TXHARQ_PDU_WAITING; // waiting for feedback
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(
        pdu_->getControlInfo());
    lteInfo->setTxNumber(transmissions_);
    lteInfo->setNdi((transmissions_ == 1) ? true : false);
    EV << "LteHarqUnitTx::extractPdu - ndi set to " << ((transmissions_ == 1) ? "true" : "false") << endl;

    LteMacPdu* extractedPdu = pdu_->dup();
    macOwner_->takeObj(extractedPdu);
    return extractedPdu;
}

bool LteHarqUnitTx::pduFeedback(HarqAcknowledgment a)
{
    EV << "LteHarqUnitTx::pduFeedback - Welcome!" << endl;
    double sample;
    bool reset = false;
    UserControlInfo *lteInfo;
    lteInfo = check_and_cast<UserControlInfo *>(pdu_->getControlInfo());
    short unsigned int dir = lteInfo->getDirection();
    unsigned int ntx = transmissions_;
    if (!(status_ == TXHARQ_PDU_WAITING))
    throw cRuntimeError("Feedback sent to an H-ARQ unit not waiting for it");

    if (a == HARQACK)
    {
        // pdu_ has been sent and received correctly
        EV << "\t pdu_ has been sent and received correctly " << endl;
        resetUnit();
        reset = true;
        sample = 0;
    }
    else if (a == HARQNACK)
    {
        sample = 1;
        if (transmissions_ == (maxHarqRtx_ + 1))
        {
            // discard
            EV << NOW << " LteHarqUnitTx::pduFeedback H-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " discarded "
            "(max retransmissions reached) : " << maxHarqRtx_ << endl;
            resetUnit();
            reset = true;
        }
        else
        {
            // pdu_ ready for next transmission
            macOwner_->takeObj(pdu_);
            status_ = TXHARQ_PDU_BUFFERED;
            EV << NOW << " LteHarqUnitTx::pduFeedbackH-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " set for RTX " << endl;
        }
    }
    else
    {
        throw cRuntimeError("LteHarqUnitTx::pduFeedback unknown feedback received from process %d , Codeword %d", acid_, cw_);
    }

    LteMacBase* ue;
    if (dir == DL)
    {
        ue = dstMac_;
    }
    else if (dir == UL)
    {
        ue = macOwner_;
    }
    else
    {
        throw cRuntimeError("LteHarqUnitTx::pduFeedback(): unknown direction");
    }

    // emit H-ARQ statistics
    switch (ntx)
    {
        case 1:
        ue->emit(harqErrorRate_1_, sample);
        break;
        case 2:
        ue->emit(harqErrorRate_2_, sample);
        break;
        case 3:
        ue->emit(harqErrorRate_3_, sample);
        break;
        case 4:
        ue->emit(harqErrorRate_4_, sample);
        break;
        default:
        break;
    }

    ue->emit(harqErrorRate_, sample);

    if (reset)
    {
        ue->emit(macPacketLoss_, sample);
        nodeB_->emit(macCellPacketLoss_, sample);
    }

    return reset;
}

bool LteHarqUnitTx::isEmpty()
{
    return (status_ == TXHARQ_PDU_EMPTY);
}

bool LteHarqUnitTx::isReady()
{
    return (status_ == TXHARQ_PDU_BUFFERED);
}

bool LteHarqUnitTx::selfNack()
{
    if (status_ == TXHARQ_PDU_WAITING)
        // wrong usage, manual nack now is dangerous (a real one may arrive too)
        throw cRuntimeError("LteHarqUnitTx::selfNack(): Trying to send self NACK to a unit waiting for feedback");

    if (status_ != TXHARQ_PDU_BUFFERED)
        throw cRuntimeError("LteHarqUnitTx::selfNack(): Trying to send self NACK to an idle or selected unit");

    transmissions_++;
    txTime_ = NOW;
    bool result = this->pduFeedback(HARQNACK);
    return result;
}

void LteHarqUnitTx::dropPdu()
{
    if (status_ != TXHARQ_PDU_BUFFERED)
        throw cRuntimeError("LteHarqUnitTx::dropPdu(): H-ARQ TX unit: cannot drop pdu if state is not BUFFERED");

    resetUnit();
}

void LteHarqUnitTx::forceDropUnit()
{
    if (status_ == TXHARQ_PDU_BUFFERED || status_ == TXHARQ_PDU_SELECTED || status_ == TXHARQ_PDU_WAITING)
    {
        delete pdu_;
        pdu_ = NULL;
    }
    resetUnit();
}

LteMacPdu *LteHarqUnitTx::getPdu()
{
    return pdu_;
}

LteHarqUnitTx::~LteHarqUnitTx()
{
    resetUnit();
}

void LteHarqUnitTx::resetUnit()
{
    transmissions_ = 0;
    pduId_ = -1;
    if(pdu_ != NULL){
        delete pdu_;
        pdu_ = NULL;
    }

    status_ = TXHARQ_PDU_EMPTY;
    pduLength_ = 0;
}
