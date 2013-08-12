// 
// This is the RLC message flowing through LTE stack.
// The relevant field is the Main Packet Sequence Number
//
packet LteRlcSdu
{
    unsigned int snoMainPacket;                        // ID of packet (sequence number)
}
