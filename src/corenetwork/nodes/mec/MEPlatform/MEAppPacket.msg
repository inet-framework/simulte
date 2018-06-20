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

packet MEAppPacket {
   
    unsigned int sno;
    simtime_t timestamp;
    
    //type of message
    string type;
    
    //communication infos
    string sourceAddress;
    string destinationAddress;
    
    //instantiation info
    string MEModuleType;			//path where to find the cModule of the MEApp to instantiate
    string MEModuleName;			//module class name of the MEApp to instantiate
    
    //resources required
    unsigned int requiredDisk;
    unsigned int requiredRam;
    double requiredCpu;
}
