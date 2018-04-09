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


#ifndef __SIMULTE_UECLUSTERIZEAPP_H_
#define __SIMULTE_UECLUSTERIZEAPP_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"

#include "apps/vehicular/v2v/v2vAlert/V2vAlertChainApp.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"


/*
 * UEClusterizeAPP
 *
 *  Sender Side task is:
 *       1) Sending START_CLUSTERIZE ClusterizePacket to the meHost (VirtualisationManager)
 *       2) Sending periodically INFO_CLUSTERIZE ClusterizeInfoPacket to the correspondent MEClusterizeApp
 *
 *  Receiver Side task is:
 *       1) Receiving CONFIG_CLUSTERIZE ClsuterizeConfigPacket from the MEClusterizeApp
 *          and configuring the V2V App, with the received configuration, by calling the V2V App public set method
 */

class UEClusterizeApp : public cSimpleModule
{
    inet::UDPSocket socket;

    int nextSnoStart_;
    int nextSnoInfo_;
    int nextSnoStop_;
    int size_;
    simtime_t period_;

    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    char* sourceSimbolicAddress;            //Car[x]
    char* destSimbolicAddress;              //meHost.virtualisationInfrastructure

    cMessage *selfStart_;
    cMessage *selfSender_;
    cMessage *selfStop_;

    cModule* v2vApp;                        //v2vApp module
    const char* v2vAppName;                 //v2vApp Class Name

    inet::IMobility *mobility;
    inet::Coord position;
    inet::Coord speed;
    inet::EulerAngles angularPosition;
    inet::EulerAngles angularSpeed;

    simsignal_t clusterizeInfoSentMsg_;
    simsignal_t clusterizeConfigRcvdMsg_;

    public:

        ~UEClusterizeApp();
        UEClusterizeApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        // sending, until receiving an ack, to the meHost (VirtualisationManager) a START_CLUSTERIZE ClusterizePacket
        // requesting to instantiate a MEClusterizeApp
        void sendClusterizeStartPacket();

        // handling ACK_START_CLUSTERIZE ClusterizePacket
        // by stopping the sendClusterizeStartPacket() and starting sendClusterizeInfoPacket()
        void handleClusterizeAckStart(ClusterizePacket*);

        // sending to the correspondent MEClusterizeApp (trhough VirtualisationManager) an INFO_CLUSTERIZE ClsuterizeInfoPacket
        // to communicate car-local informations (i.e. position, speed)
        void sendClusterizeInfoPacket();

        // sending, until receiving an ack, to the meHost (VirtualisationManager) a STOP_CLUSTERIZE ClusterizePacket
        // requesting to terminate the correspondent MEClusterizeApp
        void sendClusterizeStopPacket();

        // handling ACK_STOP_CLUSTERIZE ClusterizePacket
        // by stopping sendClusterizeStopPacket()
        void handleClusterizeAckStop(ClusterizePacket*);

        // handling CONFIG_CLUSTERIZE ClusterizeConfigPacket
        // by invoking on the v2vApp instance the public method to update its clustering configurations
        void handleClusterizeConfig(ClusterizeConfigPacket *);

};

#endif
