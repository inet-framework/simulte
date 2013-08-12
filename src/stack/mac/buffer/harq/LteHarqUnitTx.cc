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


#include "LteHarqUnitTx.h"
#include "LteMacEnb.h"

LteHarqUnitTx::LteHarqUnitTx(unsigned char acid, Codeword cw,
        LteMacBase *macOwner, LteMacBase *dstMac) {
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

    tSample_ = new TaggedSample();

    tSampleCell_ = new TaggedSample();
    tSampleCell_->module = check_and_cast<cComponent*>(macOwner_);

    if (macOwner_->getNodeType() == ENODEB) {
        tSample_->module = check_and_cast<cComponent*>(dstMac_);

        macCellPacketLoss_ = macOwner_->registerSignal("macCellPacketLossDl");

        tSampleCell_->module = check_and_cast<cComponent*>(macOwner_);

        macPacketLoss_ = dstMac_->registerSignal("macPacketLossDl");
        HarqErrorRate_ = dstMac_->registerSignal("HarqErrorRateDl");
        HarqErrorRate_1_ = dstMac_->registerSignal("HarqErrorRate_1st_Dl");
        HarqErrorRate_2_ = dstMac_->registerSignal("HarqErrorRate_2nd_Dl");
        HarqErrorRate_3_ = dstMac_->registerSignal("HarqErrorRate_3rd_Dl");
        HarqErrorRate_4_ = dstMac_->registerSignal("HarqErrorRate_4th_Dl");
    } else {
        cModule* nodeB = getMacByMacNodeId(macOwner_->getMacCellId());
        tSample_->module = check_and_cast<cComponent*>(macOwner_);

        macCellPacketLoss_ = nodeB->registerSignal("macCellPacketLossUl");
        tSampleCell_->module = check_and_cast<cComponent*>(nodeB);

        macPacketLoss_ = macOwner_->registerSignal("macPacketLossUl");
        HarqErrorRate_ = macOwner_->registerSignal("HarqErrorRateUl");
        HarqErrorRate_1_ = macOwner_->registerSignal("HarqErrorRate_1st_Ul");
        HarqErrorRate_2_ = macOwner_->registerSignal("HarqErrorRate_2nd_Ul");
        HarqErrorRate_3_ = macOwner_->registerSignal("HarqErrorRate_3rd_Ul");
        HarqErrorRate_4_ = macOwner_->registerSignal("HarqErrorRate_4th_Ul");
    }

}

void LteHarqUnitTx::insertPdu(LteMacPdu *pdu) {
    if (!pdu) {
        throw cRuntimeError("Trying to insert NULL macPdu pointer in harq unit");
    }

    // cannot insert if the unit is not idle
    if (!this->isEmpty()) {
        throw cRuntimeError("Trying to insert macPdu in already busy harq unit");
        ;
    }

    pdu_ = pdu;
    pduId_ = pdu->getId();
    transmissions_ = 0;
    status_ = TXHARQ_PDU_SELECTED;
    pduLength_ = pdu_->getByteLength();
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(
            pdu_->getControlInfo());
    lteInfo->setAcid(acid_);
    Codeword cw_old = lteInfo->getCw();
    if (cw_ != cw_old) {
        throw cRuntimeError("mismatch in cw settings");
    }
    lteInfo->setCw(cw_);

}

void LteHarqUnitTx::markSelected() {
    EV << NOW << " LteHarqUnitTx::markSelected trying to select buffer "
            << acid_ << " codeword " << cw_ << " for transmission " << endl;

    if (!(this->isReady())) {
        throw cRuntimeError(
                " ERROR acid %d codeword %d trying to select for transmission an empty buffer, aborting simulation.",
                acid_, cw_);
    }

    status_ = TXHARQ_PDU_SELECTED;
}

LteMacPdu *LteHarqUnitTx::extractPdu() {
    if (!(status_ == TXHARQ_PDU_SELECTED)) {
        throw cRuntimeError("Trying to extract macPdu from not selected H-ARQ unit");
    }

    txTime_ = NOW;
    transmissions_++;
    status_ = TXHARQ_PDU_WAITING; // waiting for feedback
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(
            pdu_->getControlInfo());
    lteInfo->setTxNumber(transmissions_);
    lteInfo->setNdi((transmissions_ == 1) ? true : false);
    EV << "LteHarqUnitTx::extractPdu - ndi set to " << ((transmissions_ == 1) ? "true" : "false") << endl;
    return pdu_;
}

bool LteHarqUnitTx::pduFeedback(HarqAcknowledgment a) {
    EV << "LteHarqUnitTx::pduFeedback - Welcome!" << endl;
    bool reset = false;
    UserControlInfo *lteInfo;
    lteInfo = check_and_cast<UserControlInfo *>(pdu_->getControlInfo());
    MacNodeId srcId = lteInfo->getSourceId();
    MacNodeId dstId = lteInfo->getDestId();
    short unsigned int dir = lteInfo->getDirection();
    unsigned int ntx = transmissions_;
    if (!(status_ == TXHARQ_PDU_WAITING)) {
        throw cRuntimeError("Feedback sent to an H-ARQ unit not waiting for it");
    }

    if (a == HARQACK) {
        // pdu_ has been sent and received correctly
        EV << "\t pdu_ has been sent and received correctly " << endl;
        delete pdu_;
        resetUnit();
        reset = true;
        tSample_->sample = 0;
        tSampleCell_->sample = 0;
    } else if (a == HARQNACK) {
        tSample_->sample = 1;
        tSampleCell_->sample = 1;
        if (transmissions_ == (maxHarqRtx_ + 1)) {
            // discard
            EV << NOW<<" LteHarqUnitTx::pduFeedback H-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
                    << pdu_->getId() << " discarded "
                    "(max retransmissions reached) : " << maxHarqRtx_ << endl;
            delete pdu_;
            resetUnit();
            reset = true;

        } else {
            // pdu_ ready for next transmission
            macOwner_->takeObj(pdu_);
            status_ = TXHARQ_PDU_BUFFERED;
            EV << NOW<<" LteHarqUnitTx::pduFeedbackH-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
                    << pdu_->getId() << " set for RTX " << endl;

        }
    } else {
        throw cRuntimeError("LteHarqUnitTx::pduFeedback unknown feedback received from process  %d , Codeword %d",
                acid_, cw_);
    }

    LteMacBase* emitter = NULL;
    if (dir == DL) {
        tSample_->id = dstId;
        emitter = dstMac_;
    } else if (dir == UL) {
        tSample_->id = srcId;
        emitter = macOwner_;
    } else {
        throw cRuntimeError("LteHarqUnitTx::pduFeedback: unknown direction");
    }

    switch (ntx) {
    case 1:
        emitter->emit(HarqErrorRate_1_, tSample_);
        break;
    case 2:
        emitter->emit(HarqErrorRate_2_, tSample_);
        break;
    case 3:
        emitter->emit(HarqErrorRate_3_, tSample_);
        break;
    case 4:
        emitter->emit(HarqErrorRate_4_, tSample_);
        break;
    default:
        break;
    }

    emitter->emit(HarqErrorRate_, tSample_);

    if (reset)
    {
        tSampleCell_->id = srcId;
        emitter->emit(macPacketLoss_, tSample_);
        macOwner_->emit(macCellPacketLoss_, tSampleCell_);
    }
    return reset;
}

bool LteHarqUnitTx::isEmpty() {
    return (status_ == TXHARQ_PDU_EMPTY);
}

bool LteHarqUnitTx::isReady() {
    return (status_ == TXHARQ_PDU_BUFFERED);
}

bool LteHarqUnitTx::selfNack() {
    if (status_ == TXHARQ_PDU_WAITING) {
        // wrong usage, manual nack now is dangerous (a real one may arrive too)
        throw cRuntimeError("LteHarqUnitTx::selfNack Trying to send self NACK to a unit waiting for feedback");
    }

    if (status_ != TXHARQ_PDU_BUFFERED) {
        throw cRuntimeError("LteHarqUnitTx::selfNack Trying to send self NACK to an idle or selected unit");
    }

    transmissions_++;
    txTime_ = NOW;
    bool result = this->pduFeedback(HARQNACK);
    return result;
}

void LteHarqUnitTx::dropPdu() {
    if (status_ != TXHARQ_PDU_BUFFERED) {
        throw cRuntimeError("LteHarqUnitTx::dropPdu H-ARQ TX unit: cannot drop pdu if state is not BUFFERED");
    }
    resetUnit();
}

void LteHarqUnitTx::forceDropUnit() {
    if (status_ == TXHARQ_PDU_BUFFERED || status_ == TXHARQ_PDU_SELECTED) {
        delete pdu_;
    }
    resetUnit();
}

LteMacPdu *LteHarqUnitTx::getPdu() {
    return pdu_;
}

LteHarqUnitTx::~LteHarqUnitTx() {
//    if (status_ == TXHARQ_PDU_BUFFERED || status_ == TXHARQ_PDU_SELECTED) {
    if (pdu_ != NULL) {
        cObject *mac = macOwner_;
        if (pdu_->getOwner() == mac) {
            delete pdu_;
        }
    }
    pdu_ = NULL;
}

void LteHarqUnitTx::resetUnit() {
    transmissions_ = 0;
    pdu_ = NULL;
    pduId_ = -1;
    status_ = TXHARQ_PDU_EMPTY;
    pduLength_ = 0;
}
