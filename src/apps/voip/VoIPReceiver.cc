//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/voip/VoIPReceiver.h"

Define_Module(VoIPReceiver);

VoIPReceiver::~VoIPReceiver()
{
    while (!mPlayoutQueue_.empty())
    {
        delete mPlayoutQueue_.front();
        mPlayoutQueue_.pop_front();
    }

    while (!mPacketsList_.empty())
    {
        delete mPacketsList_.front();
        mPacketsList_.pop_front();
    }
}

void VoIPReceiver::initialize(int stage)
{
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    emodel_Ie_ = par("emodel_Ie_");
    emodel_Bpl_ = par("emodel_Bpl_");
    emodel_A_ = par("emodel_A_");
    emodel_Ro_ = par("emodel_Ro_");

    mBufferSpace_ = par("dim_buffer");
    mSamplingDelta_ = par("sampling_time");
    mPlayoutDelay_ = par("playout_delay");

    mInit_ = true;

    int port = par("localPort");
    EV << "VoIPReceiver::initialize - binding to port: local:" << port << endl;
    if (port != -1)
    {
        socket.setOutputGate(gate("udpOut"));
        socket.bind(port);
    }

    voIPFrameLossSignal_ = registerSignal("voIPFrameLoss");
    voIPFrameDelaySignal_ = registerSignal("voIPFrameDelay");
    voIPPlayoutDelaySignal_ = registerSignal("voIPPlayoutDelay");
    voIPMosSignal_ = registerSignal("voIPMos");
    voIPTaildropLossSignal_ = registerSignal("voIPTaildropLoss");
    voIPJitterSignal_ = registerSignal("voIPJitter");
    voIPPlayoutLossSignal_ = registerSignal("voIPPlayoutLoss");
    voipReceivedThroughtput_ = registerSignal("voipReceivedThroughtput");
}

void VoIPReceiver::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;
    VoipPacket* pPacket = check_and_cast<VoipPacket*>(msg);

    if (pPacket == 0)
    {
        throw cRuntimeError("VoIPReceiver::handleMessage - FATAL! Error when casting to VoIP packet");
    }

    if (mInit_)
    {
        mCurrentTalkspurt_ = pPacket->getIDtalk();
        mInit_ = false;
    }

    if (mCurrentTalkspurt_ != pPacket->getIDtalk())
    {
        playout(false);
        mCurrentTalkspurt_ = pPacket->getIDtalk();
    }

    //emit(mFrameLossSignal,1.0);

    EV << "VoIPReceiver::handleMessage - Packet received: TALK[" << pPacket->getIDtalk() << "] - FRAME[" << pPacket->getIDframe() << " size: " << (int)pPacket->getByteLength() << " bytes]\n";

    emit(voipReceivedThroughtput_, (int)pPacket->getByteLength() );

    pPacket->setArrivalTime(simTime());
    mPacketsList_.push_back(pPacket);
}

void VoIPReceiver::playout(bool finish)
{
    if (mPacketsList_.empty())
        return;

    double sample;

    VoipPacket* pPacket = mPacketsList_.front();

    simtime_t firstPlayoutTime = pPacket->getArrivalTime() + mPlayoutDelay_;
    unsigned int firstFrameId = pPacket->getIDframe();
    unsigned int n_frames = pPacket->getNframes();
    unsigned int playoutLoss = 0;
    unsigned int tailDropLoss = 0;
    unsigned int channelLoss;

    if (finish)
    {
        PacketsList::iterator it;
        unsigned int maxId = 0;
        for (it = mPacketsList_.begin(); it != mPacketsList_.end(); it++)
            maxId = std::max(maxId, (*it)->getIDframe());
        channelLoss = maxId + 1 - mPacketsList_.size();
    }

    else
        channelLoss = pPacket->getNframes() - mPacketsList_.size();

    sample = ((double) channelLoss / (double) n_frames);
    emit(voIPFrameLossSignal_, sample);

    //Vector for managing duplicates
    bool* isArrived = new bool[pPacket->getNframes()];
    for (unsigned int y = 0; y < pPacket->getNframes(); y++)
    {
        isArrived[y] = false;
    }

    simtime_t last_jitter = 0.0;
    simtime_t max_jitter = -1000.0;

    while (!mPacketsList_.empty() /*&& pPacket->getIDtalk() == mCurrentTalkspurt*/)
    {
        pPacket = mPacketsList_.front();

        sample = SIMTIME_DBL(pPacket->getArrivalTime() - pPacket->getTimestamp());
        emit(voIPFrameDelaySignal_, sample);

        unsigned int IDframe = pPacket->getIDframe();

        pPacket->setPlayoutTime(firstPlayoutTime + IDframe * mSamplingDelta_);
        //pPacket->setPlayoutTime(firstPlayoutTime + (pPacket->getIDframe() - firstFrameId) * mSamplingDelta_);

        last_jitter = pPacket->getArrivalTime() - pPacket->getPlayoutTime();
        max_jitter = std::max(max_jitter, last_jitter);

        EV << "VoIPReceiver::playout - Jitter measured: " << last_jitter << " TALK[" << pPacket->getIDtalk() << "] - FRAME[" << pPacket->getIDframe() << "]\n";

        //Duplicates management
        if (isArrived[pPacket->getIDframe()])
        {
            EV << "VoIPReceiver::playout - Duplicated Packet: TALK[" << pPacket->getIDtalk() << "] - FRAME[" << pPacket->getIDframe() << "]\n";
            delete pPacket;
        }
        else if( last_jitter > 0.0 )
        {
            ++playoutLoss;

            EV << "VoIPReceiver::playout - out of time packet deleted: TALK[" << pPacket->getIDtalk() << "] - FRAME[" << pPacket->getIDframe() << "]\n";
            emit(voIPJitterSignal_, last_jitter);
            delete pPacket;
        }
        else
        {
            while( !mPlayoutQueue_.empty() && pPacket->getArrivalTime() > mPlayoutQueue_.front()->getPlayoutTime() )
            {
                ++mBufferSpace_;
                delete mPlayoutQueue_.front();
                mPlayoutQueue_.pop_front();
            }

            if(mBufferSpace_ > 0)
            {
                EV << "VoIPReceiver::playout - Sampleable packet inserted into buffer: TALK["<< pPacket->getIDtalk() << "] - FRAME[" << pPacket->getIDframe()
                   << "] - arrival time[" << pPacket->getArrivalTime() << "] -  sampling time[" << pPacket->getPlayoutTime() << "]\n";
                --mBufferSpace_;

                //duplicates management
                isArrived[pPacket->getIDframe()] = true;

                mPlayoutQueue_.push_back(pPacket);
            }
            else
            {
                ++tailDropLoss;
                EV << "VoIPReceiver::playout - Buffer is full, discarding packet: TALK[" << pPacket->getIDtalk() << "] - FRAME["
                   << pPacket->getIDframe() << "] - arrival time[" << pPacket->getArrivalTime() << "]\n";
                delete pPacket;
            }
        }

        mPacketsList_.pop_front();
    }

    double proportionalLoss = ((double) tailDropLoss + (double) playoutLoss + (double) channelLoss) / (double) n_frames;
    EV << "VoIPReceiver::playout - proportionalLoss " << proportionalLoss << "(tailDropLoss=" << tailDropLoss << " - playoutLoss="
       <<  playoutLoss << " - channelLoss=" << channelLoss << ")\n\n";

    double mos = eModel(mPlayoutDelay_, proportionalLoss);

//    sample = SIMmPlayoutDelay_;
    emit(voIPPlayoutDelaySignal_, mPlayoutDelay_);

    sample = ((double) playoutLoss / (double) n_frames);
    emit(voIPPlayoutLossSignal_, sample);

    sample = mos;
    emit(voIPMosSignal_, sample);

    sample = ((double) tailDropLoss / (double) n_frames);
    emit(voIPTaildropLossSignal_, sample);

    EV << "VoIPReceiver::playout - Computed MOS: eModel( " << mPlayoutDelay_ << " , " << tailDropLoss << "+" << playoutLoss << "+"
       << channelLoss << " ) = " << mos << "\n";

    EV << "VoIPReceiver::playout - Playout Delay Adaptation \n" << "\t Old Playout Delay: " << mPlayoutDelay_ << "\n\t Max Jitter Measured: "
       << max_jitter << "\n\n";

    mPlayoutDelay_ += max_jitter;
    if (mPlayoutDelay_ < 0.0)
        mPlayoutDelay_ = 0.0;
    EV << "\t New Playout Delay: " << mPlayoutDelay_ << "\n\n";

    delete[] isArrived;
}

double VoIPReceiver::eModel(simtime_t delay, double loss)
{
    double delayms = 1000 * SIMTIME_DBL(delay);

    // Compute the Id parameter
    int u = ((delayms - 177.3) > 0 ? 1 : 0);
    double id = 0.0;
    id = 0.024 * delayms + 0.11 * (delayms - 177.3) * u;

    // Packet loss p in %
    double p = loss * 100;
    // Compute the Ie,eff parameter
    double ie_eff = emodel_Ie_ + (95 - emodel_Ie_) * p / (p + emodel_Bpl_);

    // Compute the R factor
    double Rfactor = emodel_Ro_ - id - ie_eff + emodel_A_;

    // Compute the MOS value
    double mos = 0.0;

    if (Rfactor < 0)
    {
        mos = 1.0;
    }
    else if (Rfactor > 100)
    {
        mos = 4.5;
    }
    else
    {
        mos = 1 + 0.035 * Rfactor + 7 * pow(10, (double) -6) * Rfactor *
            (Rfactor - 60) * (100 - Rfactor);
    }

    mos = (mos < 1) ? 1 : mos;

    return mos;
}

void VoIPReceiver::finish()
{
    // last talkspurt playout
    playout(true);
}

