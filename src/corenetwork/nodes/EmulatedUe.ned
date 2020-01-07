// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes;

import lte.corenetwork.nodes.Ue;
import inet.linklayer.contract.IExternalNic;
import inet.common.packet.PcapRecorder;

// 
// User Equipment Module for Emulation
//
module EmulatedUe extends Ue
{
    parameters:
        
        //# Ext interface
        bool enableExtInterface = default(false);
        string extHostAddress = default("");
        
        int numPcapRecorders = default(0); // no of PcapRecorders
    
    submodules:
        
        ext: <default("ExtInterface")> like IExternalNic if enableExtInterface {
            parameters:
                @display("p=110.664,406.116,row,10;q=txQueue;i=block/ifcard");
        }

        pcapRecorder[numPcapRecorders]: PcapRecorder {
            @display("p=53,350,r,10");
        }
        
    connections allowunconnected:
        ext.upperLayerOut --> networkLayer.ifIn++ if enableExtInterface;
        ext.upperLayerIn <-- networkLayer.ifOut++ if enableExtInterface;
}
