#include <inet/common/Protocol.h>
#include <inet/common/ProtocolTag_m.h>
#include "epc/gtp/conversion.h"
#include "epc/gtp/GtpUserMsg_m.h"
#include "epc/gtp/TftControlInfo.h"




using namespace inet;
namespace gtp {
namespace modification {
TrafficFlowTemplateId removeTftControlInfo(Packet * packet) {
    TftControlInfo * tftInfo = packet->removeTag<TftControlInfo>();
    TrafficFlowTemplateId flowId = tftInfo->getTft();
    delete (tftInfo);
    return flowId;
}
}
namespace conversion {
converted copyAndPopGtpHeader(Packet * gtpPacket) {
    auto oldDatagram = new Packet(gtpPacket->getName());
    auto gtpMsg = gtpPacket->popAtFront<GtpUserMsg>();
    oldDatagram->insertAtBack(gtpPacket->peekData());
    oldDatagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(
                &Protocol::ipv4);
    return std::make_tuple(std::unique_ptr<Packet>(gtpPacket), oldDatagram,  gtpMsg->getTeid());

}
Packet * packetToGtpUserMsg(int teid, Packet * pkt) {
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
}
}
