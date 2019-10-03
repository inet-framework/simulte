//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_CHANNELMODEL_LTECHANNELMODEL_H_
#define STACK_PHY_CHANNELMODEL_LTECHANNELMODEL_H_



#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/phy/layer/LtePhyBase.h"
#include "stack/phy/packet/LteAirFrame.h"
#include <omnetpp.h>

class LteAirFrame;
class LtePhyBase;

class LteChannelModel : public cSimpleModule
{
  protected:
    unsigned int band_;
    LtePhyBase * phy_;

  public:
    virtual void setBand( unsigned int band ) = 0;// { band_ = band ;}
    virtual void setPhy( LtePhyBase * phy ) = 0;// { phy_ = phy ; }

    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual bool isCorrupted(LteAirFrame *frame, UserControlInfo* lteI) = 0;
    //TODO NOT IMPLEMENTED YET
    virtual bool isCorruptedDas(LteAirFrame *frame, UserControlInfo* lteI) = 0;
    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param move position of end point comunication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    virtual double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord) = 0;
    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    virtual double computePathLoss(double distance, double dbp, bool los) = 0;
    /*
     * Compute sir for each band for user nodeId according to multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSIR(LteAirFrame *frame, UserControlInfo* lteInfo) = 0;
    /*
     * Compute sinr for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo* lteInfo) = 0;
    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     * @param rsrpVector the received signal for each RB, if it has already been computed
     */
    virtual bool error_D2D(LteAirFrame *frame, UserControlInfo* lteInfo, const std::vector<double>& rsrpVector) = 0;
    /*
     * Compute Received useful signal for D2D transmissions
     */
    virtual std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord) = 0;
    /*
     * Compute sinr (D2D) for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo,MacNodeId peerUeId,inet::Coord peerUeCoord,MacNodeId enbId=0) = 0;
    virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId,const std::vector<double>& rsrpVector) = 0;

    virtual bool isUplinkInterferenceEnabled() { return false; }
    virtual bool isD2DInterferenceEnabled() { return false; }
};

#endif
