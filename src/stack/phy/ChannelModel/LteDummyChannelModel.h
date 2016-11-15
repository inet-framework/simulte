//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEDUMMYCHANNELMODEL_H_
#define _LTE_LTEDUMMYCHANNELMODEL_H_

#include "LteChannelModel.h"

class LteDummyChannelModel : public LteChannelModel
{
  protected:
    double per_;
    double harqReduction_;
    public:
    LteDummyChannelModel(ParameterMap& params, int band);
    virtual ~LteDummyChannelModel();
    /*
     * Compute the error probability of the transmitted packet
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual bool error(LteAirFrame *frame, UserControlInfo* lteInfo);
    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     */
    virtual double getAttenuation(MacNodeId nodeId, Direction dir, Coord coord)
    {
        return 0;
    }
    /*
     * Compute FAKE sir for each band for user nodeId according to multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSIR(LteAirFrame *frame, UserControlInfo* lteInfo);
    /*
     * Compute FAKE sinr for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo* lteInfo);
    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     * @param rsrpVector the received signal for each RB, if it has already been computed
     */
    virtual bool error_D2D(LteAirFrame *frame, UserControlInfo* lteInfo, std::vector<double> rsrpVector);
    /*
     * Compute Received useful signal for D2D transmissions
     */
    virtual std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord);
    /*
     * Compute FAKE sinr (D2D) for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord,MacNodeId enbId);
    virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord,MacNodeId enbId,std::vector<double> rsrpVector);
    //TODO
    virtual bool errorDas(LteAirFrame *frame, UserControlInfo* lteI)
    {
        throw cRuntimeError("DAS PHY LAYER TO BE IMPLEMENTED");
        return false;
    }
};

#endif
