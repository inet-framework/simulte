//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


//
// This is the RLC message flowing through LTE stack.
// The relevant field is the Main Packet Sequence Number
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class LteRlcSdu extends inet::FieldsChunk
{
    //@customize(true);
    // TODO: size 0
    chunkLength = inet::B(1); // TODO: should be a tag;
    unsigned int snoMainPacket;                        // ID of packet (sequence number)
    unsigned int lengthMainPacket;
}
