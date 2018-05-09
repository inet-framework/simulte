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

package lte.apps.vehicular.v2v;
import inet.applications.contract.IUDPApp;

//
// TODO auto-generated module
//
simple V2vClusterBaseApp like IUDPApp
{
    parameters:
                        
        int localPort = default(3000);
        int destPort = default(3000);
        string destAddress = default("");					
        int packetSize = default(10);
        
        double period @unit("s") = default(0.02s);
        double startTime @unit("s") = default(0s);
        string interfaceTableModule;
        
        @display("i=block/source");
        
        //extending eventually with signals
        
    gates:
        
        output udpOut;
        input udpIn;
}