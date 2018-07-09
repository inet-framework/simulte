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

#ifndef __SIMULTE_MEWARNINGALERTAPP_H_
#define __SIMULTE_MEWARNINGALERTAPP_H_

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEWarningAlertPacket
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "../warningAlert/packets/WarningAlertPacket_m.h"

/**
 * See MEWarningAlertApp.ned
 */
class MEWarningAlertApp : public cSimpleModule
{
    char* ueSimbolicAddress;
    char* meHostSimbolicAddress;
    inet::L3Address destAddress_;
    int size_;

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        void finish();

        void handleInfoUEWarningAlertApp(WarningAlertPacket* pkt);
        void handleInfoMEWarningAlertApp(WarningAlertPacket* pkt);
};

#endif
