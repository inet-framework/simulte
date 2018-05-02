
package lte.apps.burst;

import inet.applications.contract.IUDPApp;


simple BurstSender like IUDPApp
{
    parameters:
        int localPort = default(-1);
        int destPort = default(3000);
        string destAddress;
        double startTime @unit("s") = default(0s);
                
        int burstSize = default(10);                        // num of packets per burst
        double interBurstTime @unit("s") = default(1s);     // interval between two burst
         
        int packetSize @unit(B) = default(40B);             // size (in bytes) of one packet
        double intraBurstTime @unit("s") = default(0.01s);  // interval between packets of a burst
                
        @signal[burstSentPkt];
        @statistic[burstSentPkt](title="Sent packet ID"; unit=""; source="burstSentPkt"; record=vector);
        
        @display("i=block/source");
    gates:
        output udpOut;
        input udpIn;
}
