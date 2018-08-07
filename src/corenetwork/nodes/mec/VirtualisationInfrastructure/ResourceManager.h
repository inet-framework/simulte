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


#ifndef __SIMULTE_RESOURCEMANAGER_H_
#define __SIMULTE_RESOURCEMANAGER_H_

#include <omnetpp.h>

// ME App Modules to get resource requirements
#include "apps/mec/clusterize/MEClusterizeApp.h"

// UE/ME Packets & UE/ME App to Handle
#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"


#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_m.h"

//###########################################################################
//data structures

struct resourceMapEntry
{
    int ueAppID;
    double ram;
    double disk;
    double cpu;
};
//###########################################################################

/**
 * ResourceManager
 *
 *  The task of this class is:
 *       1) handling resource allocation for MEClusterizeApp:
 *              a) START_MEAPP --> check for resource availability & eventually allocate it
 *              b) STOP_MEAPP --> deallocate resources
 *              c) replying to VirtualisationManager by sending back START_MEAPP (when resource allocated) or with ACK_STOP_MEAPP
 */
class ResourceManager : public cSimpleModule
{
    cModule* virtualisationInfr;
    cModule* meHost;
    //maximum resources
    double maxRam;
    double maxDisk;
    double maxCPU;
    //allocated resources
    double allocatedRam;
    double allocatedDisk;
    double allocatedCPU;
    //storing resource allocation for each MEApp: resources required by UEApps
    //key = UE App ID - value resourceMapEntry
    std::map<int, resourceMapEntry> resourceMap;

    public:

        ResourceManager();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        /*
         * ------------------Default MEAppPacket handler--------------------------
         *
         */
        // allocating resources based on the amount specified in the MEAppPacket
        void handleMEAppResources(MEAppPacket*);

        /*
         * --------------------ClusterizePacket handler---------------------------
         */
        // handling START_CLUSTERIZE and STOP_CLUSTERIZE ClusterizePackets
        // by allocating or deallocating resources and sending back to VirtualisationManager the ClusterizePacket
        void handleClusterizeResources(ClusterizePacket*);

        /*
         * utility
         */
        void printResources(){
            EV << "ResourceManager::printResources - allocated Ram: " << allocatedRam << " / " << maxRam << endl;
            EV << "ResourceManager::printResources - allocated Disk: " << allocatedDisk << " / " << maxDisk << endl;
            EV << "ResourceManager::printResources - allocated CPU: " << allocatedCPU << " / " << maxCPU << endl;
        }
        void allocateResources(double ram, double disk, double cpu){
            allocatedRam += ram;
            allocatedDisk += disk;
            allocatedCPU += cpu;
            printResources();
        }
        void deallocateResources(double ram, double disk, double cpu){
            allocatedRam -= ram;
            allocatedDisk -= disk;
            allocatedCPU -= cpu;
            printResources();
        }
};

#endif
