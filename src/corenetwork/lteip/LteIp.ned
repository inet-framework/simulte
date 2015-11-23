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
// LteIp implements a simplified version of the IP protocol for the LTE model. 
// The protocol header is represented by the IPDatagram message class.
//
// <b>Interfacing with higher layer protocols</b>
//
// To send a packet over LteIp from a higher layer protocol, the higher layer
// module should fill in an IPControlInfo object, attach it to the packet 
// with cMessage's setControlInfo() method, the send it to the LteIp module.
//
// When LteIp sends up a packet to a higher layer protocol, it will also attach
// an IPControlInfo to the packet, with the source and destination IP address,
// etc. of the IP datagram in which the packet arrived.
//
// When the higher layer is TCP, the processing delay should be kept 
// low ( i.e. < 1s ), otherwise it is possible to get a runtime error.
//
// LteIp can serve several higher-layer protocols. When delivering packets
// to them, the output gate is determined from the Protocol field in the
// IP header. The protocol-to-gateindex mapping must be given in the
// protocolMapping string parameter, in the following format:
// <pre>
// protocolnumber:gateindex, protocolnumber:gateindex, ...
// </pre>
//
// The recommended setting for LteIp is:
// - \TCP (6) : 0
// - \UDP (17) : 1
//
// That is, <tt>protocolMapping="6:0,17:1"</tt>
//
// Note that on eNodeB nodes no protocol mapping is necessary,
// because the LteIp module on eNodeB forwards IP Datagrams
// between an LteIp peer and the Lte Stack. Leaving 
// protocolMapping parameter to its default ("") its enough.
// 
// <b> Data flow </b>
//
// The behaviour of this LteIp module, as regards the data flow,
// depends on the nodeType parameter:
// - "INTERNET" : Transport Layer <--> LteIp Peer.
// - "ENODEB"   : LteIp Peer <--> LTE Stack
// - "UE"       : Transport Layer <--> LTE Stack
//
// When an IP Datagram is forwarded to the LTE Stack, an
// LteStackControlInfo is attached. 
//
simple LteIp
{
    parameters:
        @display("i=block/routing");
         double procDelay @unit("s") = default(0s); // processing delay 
         string localAddr = default("127.0.0.1");   // default src address (on enodeb can be left default)
        int timeToLive = default(32);              // default time to live
        string protocolMapping = default("");      // mapping between transportIn[]/Out[] indexes and protocol numbers
        string nodeType;                           // can be "internet", "enodeb", "ue"
        
    gates:
 
        // Gates to transport layers (unconnected in eNodeB)
        input transportIn[] @labels(IPControlInfo/down,TCPSegment,UDPPacket);
        output transportOut[] @labels(IPControlInfo/up,TCPSegment,UDPPacket);
 
        // Gate to Lte stack (unconnected in Internet node)
        inout stackLte @labels(IPDatagram);
 
        // Gates to LteIp peer (unconnected in UE)
        input ifIn @labels(IPDatagram);
        output ifOut @labels(IPDatagram);
}
