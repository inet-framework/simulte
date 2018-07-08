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

//
// IceAlert Packet used by UEIceAlertApp MEIceAlertApp and MEIceAlertService
//


cplusplus {{
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_m.h"
}}
packet MEAppPacket;


packet IceAlertPacket extends MEAppPacket{   
    
    //meaningfull for INFO_UEAPP type  -> used by the MEIceAlertService
    int ueOmnetID; 
    double positionX;
    double positionY;
    double positionZ;
    
    //meaningfull for INFO_MEAPP type -> used by the UEIceAlertApp to change its color
    bool danger;
}
