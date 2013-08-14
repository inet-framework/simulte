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

#ifndef _LTE_LTECHANNELMODEL_H_
#define _LTE_LTECHANNELMODEL_H_
#include "LteCommon.h"
#include "LteControlInfo.h"

class LteAirFrame;

class LteChannelModel
{
  protected:
    unsigned int band_;
    public:
    LteChannelModel(unsigned int band);
    virtual ~LteChannelModel();
    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual bool error(LteAirFrame *frame, UserControlInfo* lteI)=0;
    //TODO NOT IMPLEMENTED YET
    virtual bool errorDas(LteAirFrame *frame, UserControlInfo* lteI)=0;
    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param move position of end point comunication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    virtual double getAttenuation(MacNodeId nodeId, Direction dir, Coord coord)=0;
    /*
     * Compute sir for each band for user nodeId according to multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSIR(LteAirFrame *frame, UserControlInfo* lteInfo)=0;
    /*
     * Compute sinr for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo* lteInfo)=0;
};

#endif
