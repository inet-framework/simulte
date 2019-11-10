#ifndef _ABSTRACT_LTE_GTP_USER_H_
#define _ABSTRACT_LTE_GTP_USER_H_

#include <omnetpp.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/InterfaceEntry.h>
#include "epc/gtp_common.h"

class AbstractGtpUser : public omnetpp::cSimpleModule {
public:
    virtual ~AbstractGtpUser() = 0;
protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    virtual void handleFromTrafficFlowFilter(inet::Packet * packet) = 0;
    virtual void handleFromUdp(inet::Packet * packet) = 0;
    inet::Packet* convertToGtpUserMsg(int teid, inet::Packet *packet);


    inet::UdpSocket socket_;
    int localPort_;
    unsigned int tunnelPeerPort_;
    inet::InterfaceEntry *ie_;
    EpcNodeType selectOwnerType(const char * type);
    inet::InterfaceEntry *detectInterface();
};
#endif
