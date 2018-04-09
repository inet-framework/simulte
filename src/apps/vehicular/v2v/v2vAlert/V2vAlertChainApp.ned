//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  @author Angelo Buono
//

package lte.apps.vehicular.v2v.v2vAlert;

import inet.applications.contract.IUDPApp;

//
// TODO auto-generated module
//
simple V2vAlertChainApp like IUDPApp
{
    parameters:
                        
        int localPort = default(3000);
        int destPort = default(3000);
        string destAddress = default("");					
        int packetSize = default(10);
        
        double period @unit("s") = default(0.02s);
        double startTime @unit("s") = default(0s);
        string interfaceTableModule;
        
        //sender side signals       
        @signal[v2vAlertChainSentMsg];
        @statistic[v2vAlertChainSentMsg](title="V2vAlertChain messages sent"; unit=""; source="v2vAlertChainSentMsg"; record=sum,vector);

        //receiver side signals
        @signal[v2vAlertChainDelay];
        @statistic[v2vAlertChainDelay](title="V2vAlertChainDelay Message Delay"; unit="s"; source="v2vAlertChainDelay"; record=mean,vector);
           
        @signal[v2vAlertChainRcvdMsg]; 
        @statistic[v2vAlertChainRcvdMsg](title="V2vAlertChainRcvdMsg Messages Received"; unit="s"; source="v2vAlertChainRcvdMsg"; record=sum,vector);
              
        @display("i=block/source");
        
    gates:
        
        output udpOut;
        input udpIn;
}