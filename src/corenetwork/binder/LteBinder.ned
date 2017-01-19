// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.binder;

// 
// This is the LTE Binder. At the moment it stores a table containing
// for each ip address the corresponding node id. It is used by the sender
// to find out the Id of the destination node
//
simple LteBinder 
{
    parameters:
        
        // number of logical bands
        int numBands = default(6);
         
        //QoS Parameters (strings)
        string priority = "2 4 3 5 1 6 7 8 9";
        string packetDelayBudget = "0.1 0.15 0.05 0.3 0.1 0.3 0.1 0.3 0.3";          // @unit(s)
        string packetErrorLossRate = "1e-2 1e-3 1e-3 1e-6 1e-6 1e-6 1e-3 1e-6 1e-6";
        
        @display("i=block/cogwheel");
        
   
        //# XML scenario configuration support
        //#
        //#
        // xml traffic = default(xmldoc("traffic.xml"));
        
}
