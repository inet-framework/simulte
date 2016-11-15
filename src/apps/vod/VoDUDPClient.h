//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
//

#ifndef _LTE_VODUDPCLIENT_H_
#define _LTE_VODUDPCLIENT_H_

#include <omnetpp.h>
#include <string.h>
#include <fstream>
#include "VoDPacket_m.h"
#include "VoDUDPStruct.h"
#include "UDPControlInfo_m.h"
#include "UDPSocket.h"
#include "L3AddressResolver.h"
#include "lterecorder.h"

using namespace std;

class VoDUDPClient : public cSimpleModule
{
    inet::UDPSocket socket;
    fstream outfile;

  public:
    simsignal_t tptLayer0_;
    simsignal_t tptLayer1_;
    simsignal_t tptLayer2_;
    simsignal_t tptLayer3_;
    TaggedSample* tSample_;
    simsignal_t delayLayer0_;
    simsignal_t delayLayer1_;
    simsignal_t delayLayer2_;
    simsignal_t delayLayer3_;

  protected:

    virtual void initialize(int stage);
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
    virtual void receiveStream(VoDPacket *msg);
};

#endif

