#include <memory>
#include <utility>

#include <inet/common/packet/Packet.h>

#include "epc/gtp_common.h"


namespace gtp {
    namespace modification {
        TrafficFlowTemplateId removeTftControlInfo(inet::Packet * packet);
    }
    namespace conversion {
        enum {GTP_PACKET, ORIGINAL_DATAGRAM, TEID};
        using converted = std::tuple<std::unique_ptr<inet::Packet>, inet::Packet*, TunnelEndpointIdentifier>;
        converted copyAndPopGtpHeader(inet::Packet * gtpPacket);
        inet::Packet * packetToGtpUserMsg(int teid, inet::Packet * pkt);
    }
}
