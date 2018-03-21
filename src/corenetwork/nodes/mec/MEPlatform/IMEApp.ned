//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//	@author	Angelo Buono
//


package lte.corenetwork.nodes.mec.MEPlatform;

//
// Template for ME Services.
//

moduleinterface IMEApp
{
    parameters:
        @display("i=block/app");
        
        double ram @unit("B");			//Bytes
        double disk @unit("B");			//Bytes
        double cpu;						//percentage
        
    gates:
        
        input virtualisationInfrastructureIn;
        output virtualisationInfrastructureOut;
}



