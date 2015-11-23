// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.lteip;

//
// This module implements a queue used to transmit packet
// over a finite bandwidth channel.
//
// The module should be attached to the LteIp
// output gate towards the LteIp peer (peerLteIp$o).
// The module sends a packet over the internet
// channel only if the channel is not busy transmitting
// a previous packet.
// If the channel is busy, the packet is stored in a queue.
// The size of the queue is configurable through queueSize
// parameter ( 0 = infinite ).
//
simple InternetQueue {
    parameters:
        @display("i=block/queue");
        int queueSize = default(100);        // 0 means infinite
    
    gates:
        input  lteIpIn;                        // input gate to receive packets from LteIP
        output internetChannelOut;             // output gate towards datarate channel
}
