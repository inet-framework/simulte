//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  @author Angelo Buono
//

#ifndef __SIMULTE_MEICEALERTAPP_H_
#define __SIMULTE_MEICEALERTAPP_H_

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//IceAlertPacket
#include "apps/mec/iceAlert/packets/IceAlertPacket_m.h"
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_Types.h"

/**
 * See MEIceAlertApp.ned
 */
class MEIceAlertApp : public cSimpleModule
{
    char* sourceSimbolicAddress;
    char* destSimbolicAddress;
    inet::L3Address destAddress_;
    int size_;

    public:
        ~MEIceAlertApp();
        MEIceAlertApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        void finish();

        void handleInfoUEIceAlertApp(IceAlertPacket* pkt);
        void handleInfoMEIceAlertApp(IceAlertPacket* pkt);
};

#endif
