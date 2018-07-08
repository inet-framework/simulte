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

#ifndef __SIMULTE_RADIONETWORKINFORMATION_H_
#define __SIMULTE_RADIONETWORKINFORMATION_H_

#include <string>

#include "corenetwork/binder/LteBinder.h"
#include "common/LteCommon.h"

using namespace omnetpp;

/**
 * RadioNetworkInformation is a General Service derived by RadioNetworkInformation feature defined in ETSI MEC 002
 *
 * This service provides:
 *                          1) retrieving the txPower for an UE under the eNodeB connected with this ME Host
 *                          2) retrieving the CQI for an UE under the eNodeB connected with this ME Host
 */
class RadioNetworkInformation : public cSimpleModule
{
    //LteBinder (oracle module)
    LteBinder* binder_;

    //parent modules
    cModule* mePlatform;
    cModule* meHost;

    //eNodeB connected to the ME Host
    std::vector<std::string> enb;

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

    public:
        //return txPwr value from the UeInfo structure in the binder
        double getUETxPower(std::string car);

        //return the cqi for the given car
        Cqi getUEcqi(std::string car);
};

#endif
