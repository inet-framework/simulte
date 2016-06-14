//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteHarqUnitTxD2D.h"
#include "LteMacEnb.h"

LteHarqUnitTxD2D::LteHarqUnitTxD2D(unsigned char acid, Codeword cw, LteMacBase *macOwner, LteMacBase *dstMac)
    : LteHarqUnitTx(acid, cw, macOwner, dstMac)
{
    if (macOwner_->getNodeType() == UE)
    {
        if (dstMac_ != nodeB_)  // D2D
        {
            macPacketLossD2D_ = macOwner_->registerSignal("macPacketLossUlD2D");
            macCellPacketLossD2D_ = nodeB_->registerSignal("macCellPacketLossUlD2D");
            harqErrorRateD2D_ = dstMac_->registerSignal("harqErrorRateD2D");
            harqErrorRateD2D_1_ = dstMac_->registerSignal("harqErrorRate_1st_D2D");
            harqErrorRateD2D_2_ = dstMac_->registerSignal("harqErrorRate_2nd_D2D");
            harqErrorRateD2D_3_ = dstMac_->registerSignal("harqErrorRate_3rd_D2D");
            harqErrorRateD2D_4_ = dstMac_->registerSignal("harqErrorRate_4th_D2D");
        }
    }
}

bool LteHarqUnitTxD2D::pduFeedback(HarqAcknowledgment a)
{
    EV << "LteHarqUnitTxD2D::pduFeedback - Welcome!" << endl;
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
        delete pdu_;
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
            EV << NOW << " LteHarqUnitTxD2D::pduFeedback H-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " discarded "
            "(max retransmissions reached) : " << maxHarqRtx_ << endl;
            delete pdu_;
            resetUnit();
            reset = true;
        }
        else
        {
            // pdu_ ready for next transmission
            macOwner_->takeObj(pdu_);
            status_ = TXHARQ_PDU_BUFFERED;
            EV << NOW << " LteHarqUnitTxD2D::pduFeedbackH-ARQ process  " << (unsigned int)acid_ << " Codeword " << cw_ << " PDU "
               << pdu_->getId() << " set for RTX " << endl;
        }
    }
    else
    {
        throw cRuntimeError("LteHarqUnitTxD2D::pduFeedback unknown feedback received from process %d , Codeword %d", acid_, cw_);
    }

    LteMacBase* ue;
    if (dir == DL || dir == D2D)
    {
        ue = dstMac_;
    }
    else if (dir == UL)
    {
        ue = macOwner_;
    }
    else
    {
        throw cRuntimeError("LteHarqUnitTxD2D::pduFeedback(): unknown direction");
    }

    // emit H-ARQ statistics
    if (dir == D2D)
    {
        switch (ntx)
        {
            case 1:
            ue->emit(harqErrorRateD2D_1_, sample);
            break;
            case 2:
            ue->emit(harqErrorRateD2D_2_, sample);
            break;
            case 3:
            ue->emit(harqErrorRateD2D_3_, sample);
            break;
            case 4:
            ue->emit(harqErrorRateD2D_4_, sample);
            break;
            default:
            break;
        }

        ue->emit(harqErrorRateD2D_, sample);

        if (reset)
        {
            ue->emit(macPacketLossD2D_, sample);
            nodeB_->emit(macCellPacketLossD2D_, sample);
        }
    }
    else
    {
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
    }

    return reset;
}

LteHarqUnitTxD2D::~LteHarqUnitTxD2D()
{
}
