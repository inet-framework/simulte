#include "epc/gtp/AbstractGtpUser.h"

#include <inet/networklayer/contract/IInterfaceTable.h>
#include <inet/common/ModuleAccess.h>
#include "epc/gtp/GtpUserMsg_m.h"

using namespace omnetpp;
using namespace inet;

AbstractGtpUser::~AbstractGtpUser() {}

EpcNodeType AbstractGtpUser::selectOwnerType(const char * type) {
    EV << "GtpUserSimplified::selectOwnerType - setting owner type to " << type << endl;
    if(strcmp(type,"ENODEB") == 0)
        return ENB;
    else if(strcmp(type,"PGW") == 0)
        return PGW;
    else if(strcmp(type,"SGW") == 0)
        return SGW;
    else
        error("GtpUser::selectOwnerType - unknown owner type [%s]. Aborting...",type);

    // should never be reached - return default value in order to avoid compile warnings
    return ENB;
}

InterfaceEntry *AbstractGtpUser::detectInterface()
{
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    const char *interfaceName = par("ipOutInterface");
    InterfaceEntry *ie = nullptr;

    if (strlen(interfaceName) > 0) {
        ie = ift->getInterfaceByName(interfaceName);
        if (ie == nullptr)
            throw cRuntimeError("Interface \"%s\" does not exist", interfaceName);
    }

    return ie;
}

void AbstractGtpUser::handleMessage(cMessage *msg)
{
    Packet* packet = check_and_cast<Packet *>(msg);
    if (strcmp(msg->getArrivalGate()->getFullName(), "trafficFlowFilterGate") == 0)
    {
        EV << "AbstractGtpUser::handleMessage - message from trafficFlowFilter" << endl;
        handleFromTrafficFlowFilter(packet);
    }
    else if(strcmp(msg->getArrivalGate()->getFullName(),"socketIn")==0)
    {
        EV << "AbstractGtpUser::handleMessage - message from udp layer" << endl;

        handleFromUdp(packet);
    }
}

Packet *AbstractGtpUser::convertToGtpUserMsg(int teid, Packet * pkt) {
    auto header = makeShared<GtpUserMsg>();
    header->setTeid(teid);
    header->setChunkLength(B(8));
    auto gtpMsg = new Packet(pkt->getName());
    gtpMsg->insertAtFront(header);
    auto data = pkt->peekData();
    gtpMsg->insertAtBack(data);
    delete pkt;
    return gtpMsg;
}



