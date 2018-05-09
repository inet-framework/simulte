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


#include "RadioNetworkInformation.h"

Define_Module(RadioNetworkInformation);

void RadioNetworkInformation::initialize(int stage)
{
    EV << "RadioNetworkInformation::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    binder_ = getBinder();

    mePlatform = getParentModule();

    if(mePlatform != NULL){
        meHost = mePlatform->getParentModule();
    }
    else{
        EV << "RadioNetworkInformation::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("RadioNetworkInformation::initialize - \tFATAL! Error when getting getParentModule()");
    }

    int nENB = meHost->gateSize("pppENB");

    for(int i=0; i<nENB; i++)
        EV <<"RadioNetworkInformation::initialize - gate owner: "<< meHost->gate("pppENB$o", i)->getPathEndGate()->getOwnerModule()->getParentModule()->getParentModule()->getFullName() << endl;
}

void RadioNetworkInformation::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
