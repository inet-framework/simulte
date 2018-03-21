//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMULTE_UECLUSTERIZESENDER_H_
#define __SIMULTE_UECLUSTERIZESENDER_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/vehicular/v2v/v2vAlert/packets/V2vAlertPacket_m.h"

/**
 * V2vAlertReceiver
 *
 *  The task of this class is:
 *       1) Receiving the V2vAlertPacket
 */

class V2vAlertReceiver : public cSimpleModule
{
        inet::UDPSocket socket;

        simsignal_t v2vAlertDelay_;
        simsignal_t v2vAlertRcvdMsg_;

        //adding a reference to V2vAlertSender to trigger the forward of an AlertPacket received...

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
};

#endif
