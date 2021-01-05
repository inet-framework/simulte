//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

import common.LteCommonEnum;
import common.LteCommon;
import inet.common.packet.chunk.Chunk;

//packet D2DModeSwitchNotification
class D2DModeSwitchNotification extends inet::FieldsChunk
{
    chunkLength = inet::B(1); // TODO: size 0
    unsigned int peerId;
    bool txSide;
    LteD2DMode oldMode;
    LteD2DMode newMode;
    bool oldConnection;
    bool interruptHarq;
    bool clearRlcBuffer;
}
