//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteDummyChannelModel.h"

LteDummyChannelModel::LteDummyChannelModel(ParameterMap& params, int band) :
    LteChannelModel(band)
{
    //default value
    ParameterMap::iterator it = params.find("per");
    if (it != params.end())
    {
        per_ = params["per"].doubleValue();
        EV << "Packet Error Probability loaded from config file: " << per_ << endl;
    }
    else
    per_=0.1;
    if (per_ < 0 || per_ > 1)
        throw cRuntimeError("Wrong PER value %g: should be smaller than 1 and greater than 0", per_);

    it = params.find("harqReduction");
    if (it != params.end())
    {
        harqReduction_ = params["harqReduction"].doubleValue();
        EV << "Harq reduction loaded from config file: " << harqReduction_ << endl;
    }
    else
    harqReduction_=0.3;
}

LteDummyChannelModel::~LteDummyChannelModel()
{
    // TODO Auto-generated destructor stub
}

std::vector<double> LteDummyChannelModel::getSINR(LteAirFrame *frame, UserControlInfo* lteInfo)
{
    std::vector<double> tmp;
    tmp.push_back(10000);
    // fake SINR is needed by das (to decide which antenna set are used by the terminal)
    // and handhover function to decide if the terminal should trigger the hanhover
    return tmp;
}

std::vector<double> LteDummyChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord)
{
    std::vector<double> tmp;
    tmp.push_back(10000);
    return tmp;
}

std::vector<double> LteDummyChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord,MacNodeId enbId)
{
    std::vector<double> tmp;
    tmp.push_back(10000);
    // fake SINR is needed by das (to decide which antenna set are used by the terminal)
    // and handhover function to decide if the terminal should trigger the hanhover
    return tmp;
}

std::vector<double> LteDummyChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord,MacNodeId enbId,std::vector<double> rsrpVector)
{
    std::vector<double> tmp;
    tmp.push_back(10000);
    // fake SINR is needed by das (to decide which antenna set are used by the terminal)
    // and handhover function to decide if the terminal should trigger the hanhover
    return tmp;
}

std::vector<double> LteDummyChannelModel::getSIR(LteAirFrame *frame, UserControlInfo* lteInfo)
{
    std::vector<double> tmp;
    tmp.push_back(10000);
    // fake SIR is needed by das (to decide which antenna set are used by the terminal)
    // and handhover function to decide if the terminal should trigger the hanhover
    return tmp;
}

bool LteDummyChannelModel::error(LteAirFrame *frame, UserControlInfo* lteInfo)
{
    // Number of RTX
    unsigned char nTx = lteInfo->getTxNumber();
    //Consistency check
    if (nTx == 0)
        throw cRuntimeError("Number of tx should not be 0");

    // compute packet error rate according to number of retransmission
    // and the harq reduction parameter
    double totalPer = per_ * pow(harqReduction_, nTx - 1);
    //Throw random variable
    double er = uniform(getEnvir()->getRNG(0),0.0, 1.0);

    if (er <= totalPer)
    {
        EV << "This is NOT your lucky day (" << er << " < " << totalPer
           << ") -> do not receive." << endl;
        // Signal too weak, we can't receive it
        return false;
    }
        // Signal is strong enough, receive this Signal
    EV << "This is your lucky day (" << er << " > " << totalPer
       << ") -> Receive AirFrame." << endl;
    return true;
}

bool LteDummyChannelModel::error_D2D(LteAirFrame *frame, UserControlInfo* lteInfo,std::vector<double> rsrpVector)
{
    // Number of RTX
    unsigned char nTx = lteInfo->getTxNumber();
    //Consistency check
    if (nTx == 0)
        throw cRuntimeError("Number of tx should not be 0");

    // compute packet error rate according to number of retransmission
    // and the harq reduction parameter
    double totalPer = per_ * pow(harqReduction_, nTx - 1);
    //Throw random variable
    double er = uniform(getEnvir()->getRNG(0),0.0, 1.0);

    if (er <= totalPer)
    {
        EV << "This is NOT your lucky day (" << er << " < " << totalPer
           << ") -> do not receive." << endl;
        // Signal too weak, we can't receive it
        return false;
    }
        // Signal is strong enough, receive this Signal
    EV << "This is your lucky day (" << er << " > " << totalPer
       << ") -> Receive AirFrame." << endl;
    return true;
}
