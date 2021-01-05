//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
//
// TODO generated message class
//
class LteSchedulingGrant extends inet::FieldsChunk
{
    @customize(true);  // see the generated C++ header for more info
    chunkLength = inet::B(1); // TODO: size 0
    bool periodic;
    unsigned int period;
    unsigned int expiration;
       // blocks granted on all Remotes, all Bands
    unsigned int totalGrantedBlocks;
    // number of codewords
    unsigned int codewords;
    // granted bytes per codeword
    abstract unsigned int grantedCwBytes[];

    //#  in LteSchedulingGrant.h
    //# const UserTxParams* userTxParams;
    //# RbMap grantedBlocks;
    //#
}
