//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This is the RLC Pdu
// flowing through LTE stack: basically
// it contains the fields of a fragment
// that receiver needs in order to
// correctly decode the original packet
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class LteRlcPdu extends inet::FieldsChunk
{
    // TODO: size 0
    chunkLength = inet::B(1); // TODO: should be a tag;
    unsigned int totalFragments;    // Total number of fragments
    unsigned int snoFragment;        // Sequence number of this fragment
    unsigned int snoMainPacket;        // ID of packet (sequence number)
}

// new data indication for MAC
class LteRlcPduNewData extends LteRlcPdu
{
}