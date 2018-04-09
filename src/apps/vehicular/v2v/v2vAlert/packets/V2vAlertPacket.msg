//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


//
// Packet used by the V2vAlertSender and received by V2vAlertReceiver.

packet V2vAlertPacket {
    
    unsigned int sno;
    simtime_t timestamp;
    
    string sourceAddress;
    string destinationAddress;
    
    string forwarderAddress;
}
