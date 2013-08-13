//
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

cplusplus {{
#include "IPvXAddress.h"
}};

struct IPvXAddress;

message M1Message {
        IPvXAddress clientAddr;   // client address
        int clientPort;           // client UDP port
        long numPkSent;           // number of packets sent
}
