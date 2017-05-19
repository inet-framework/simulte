//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

cplusplus {{
#include "inet/networklayer/common/L3Address.h"
}};

struct inet::L3Address;

message M1Message {
        inet::L3Address clientAddr;   // client address
        int clientPort;           // client UDP port
        long numPkSent;           // number of packets sent
}
