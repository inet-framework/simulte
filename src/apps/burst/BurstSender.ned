
package lte.apps.burst;

import inet.applications.contract.IApp;


simple BurstSender like IApp
{
    parameters:
        int localPort = default(-1);
        int destPort = default(3000);
        string destAddress;
        double startTime @unit("s") = default(0s);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
                
        int burstSize = default(10);                        // num of packets per burst
        double interBurstTime @unit("s") = default(1s);     // interval between two burst
         
        int packetSize @unit(B) = default(40B);             // size (in bytes) of one packet
        double intraBurstTime @unit("s") = default(0.01s);  // interval between packets of a burst
                
        @signal[burstSentPkt];
        @statistic[burstSentPkt](title="Sent packet ID"; unit=""; source="burstSentPkt"; record=vector);
        
        @display("i=block/source");
    gates:
		output socketOut;
        input socketIn;
}
