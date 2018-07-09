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

#ifndef __SIMULTE_UEWARNINGALERTAPP_H_
#define __SIMULTE_UEWARNINGALERTAPP_H_

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "corenetwork/binder/LteBinder.h"

//inet mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

//WarningAlertPacket
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "../warningAlert/packets/WarningAlertPacket_m.h"

/**
 * See UEWarningAlertApp.ned
 */
class UEWarningAlertApp : public cSimpleModule
{
    //communication
    inet::UDPSocket socket;
    int size_;
    simtime_t period_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    char* sourceSimbolicAddress;            //Ue[x]
    char* destSimbolicAddress;              //meHost.virtualisationInfrastructure

    // mobility informations
    cModule* ue;
    inet::IMobility *mobility;
    inet::Coord position;

    //resources required
    int requiredRam;
    int requiredDisk;
    double requiredCpu;

    //scheduling
    cMessage *selfStart_;
    cMessage *selfSender_;
    cMessage *selfStop_;

    public:
        ~UEWarningAlertApp();
        UEWarningAlertApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        void sendStartMEWarningAlertApp();
        void sendInfoUEWarningAlertApp();
        void sendStopMEWarningAlertApp();

        void handleAckStartMEWarningAlertApp(MEAppPacket* pkt);
        void handleInfoMEWarningAlertApp(MEAppPacket* pkt);
        void handleAckStopMEWarningAlertApp(MEAppPacket* pkt);
};

#endif
