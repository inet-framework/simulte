//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LtePhyEnbD2D.h"
#include "LteFeedbackPkt.h"
#include "LteCommon.h"
#include "DasFilter.h"

Define_Module(LtePhyEnbD2D);

LtePhyEnbD2D::LtePhyEnbD2D()
{
}

LtePhyEnbD2D::~LtePhyEnbD2D()
{
}

void LtePhyEnbD2D::initialize(int stage)
{
    LtePhyEnb::initialize(stage);
    if (stage == 0)
        enableD2DCqiReporting_ = par("enableD2DCqiReporting");
}

void LtePhyEnbD2D::requestFeedback(UserControlInfo* lteinfo, LteAirFrame* frame, LteFeedbackPkt* pkt)
{
    EV << NOW << " LtePhyEnbD2D::requestFeedback " << endl;
    //get UE Position
    Coord sendersPos = lteinfo->getCoord();
    deployer_->setUePosition(lteinfo->getSourceId(), sendersPos);

    //Apply analog model (pathloss)
    //Get snr for UL direction
    std::vector<double> snr = channelModel_->getSINR(frame, lteinfo);
    FeedbackRequest req = lteinfo->feedbackReq;
    //Feedback computation
    fb_.clear();
    //get number of RU
    int nRus = deployer_->getNumRus();
    TxMode txmode = req.txMode;
    FeedbackType type = req.type;
    RbAllocationType rbtype = req.rbAllocationType;
    std::map<Remote, int> antennaCws = deployer_->getAntennaCws();
    unsigned int numPreferredBand = deployer_->getNumPreferredBands();
    Direction dir = UL;
    while (dir != UNKNOWN_DIRECTION)
    {
        //for each RU is called the computation feedback function
        if (req.genType == IDEAL)
        {
            fb_ = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                antennaCws, numPreferredBand, IDEAL, nRus, snr,
                lteinfo->getSourceId());
        }
        else if (req.genType == REAL)
        {
            RemoteSet::iterator it;
            fb_.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb_[(*it)].resize((int) txmode);
                fb_[(*it)][(int) txmode] =
                lteFeedbackComputation_->computeFeedback(*it, txmode,
                    type, rbtype, antennaCws[*it], numPreferredBand,
                    REAL, nRus, snr, lteinfo->getSourceId());
            }
        }
        // the reports are computed only for the antenna in the reporting set
        else if (req.genType == DAS_AWARE)
        {
            RemoteSet::iterator it;
            fb_.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb_[(*it)] = lteFeedbackComputation_->computeFeedback(*it, type,
                    rbtype, txmode, antennaCws[*it], numPreferredBand,
                    DAS_AWARE, nRus, snr, lteinfo->getSourceId());
            }
        }
        if (dir == UL)
        {
            pkt->setLteFeedbackDoubleVectorUl(fb_);
            //Prepare  parameters for next loop iteration - in order to compute SNR in DL
            lteinfo->setTxPower(txPower_);
            lteinfo->setDirection(DL);

            //Get snr for DL direction
            snr = channelModel_->getSINR(frame, lteinfo);

            dir = DL;
        }
        else if (dir == DL)
        {
            pkt->setLteFeedbackDoubleVectorDl(fb_);

            if (enableD2DCqiReporting_)
            {
                // compute D2D feedback for all possible peering UEs
                std::vector<UeInfo*>* ueList = binder_->getUeList();
                std::vector<UeInfo*>::iterator it = ueList->begin();
                for (; it != ueList->end(); ++it)
                {
                    MacNodeId peerId = (*it)->id;
                    if (peerId != lteinfo->getSourceId() && binder_->checkD2DCapability(lteinfo->getSourceId(), peerId))
                    {
                         // the source UE might communicate with this peer using D2D, so compute feedback

                         // retrieve the position of the peer
                         Coord peerCoord = (*it)->phy->getCoord();

                         // get SINR for this link
                         snr = channelModel_->getSINR_D2D(frame, lteinfo, peerId, peerCoord, nodeId_);

                         // compute the feedback for this link
                         fb_ = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                                 antennaCws, numPreferredBand, IDEAL, nRus, snr,
                                 lteinfo->getSourceId());

                         pkt->setLteFeedbackDoubleVectorD2D(peerId, fb_);
                    }
                }
            }
            dir = UNKNOWN_DIRECTION;
        }
    }
    EV << "LtePhyEnbD2D::requestFeedback : Pisa Feedback Generated for nodeId: "
       << nodeId_ << " with generator type "
       << fbGeneratorTypeToA(req.genType) << " Fb size: " << fb_.size() << endl;
}
