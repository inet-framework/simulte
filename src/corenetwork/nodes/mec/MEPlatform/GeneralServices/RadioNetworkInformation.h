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

#include <omnetpp.h>
#include <string>

#include "corenetwork/binder/LteBinder.h"
#include "common/LteCommon.h"

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class RadioNetworkInformation : public cSimpleModule
{

    LteBinder* binder_;

    cModule* mePlatform;
    cModule* meHost;

    //eNodeB connected to the ME Host
    //
    std::vector<std::string> enb;



    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

    public:

        //return txPwr value from the UeInfo structure in the binder
        //
        double getUETxPower(std::string car);


        //return the cqi for the given car
        //
        Cqi getUEcqi(std::string car);
};

#endif
