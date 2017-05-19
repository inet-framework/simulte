//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


cplusplus {{
    #include "common/LteCommon.h"
    typedef std::list<X2NodeId> DestinationIdList;
}}

class noncobject DestinationIdList;

//
// X2 Control Info usage:
//
// The X2 Control Info handles communication between the X2 Manager and its users.
// It is attached to the X2InformationElement and contains:
// - The X2 ID of the source eNodeB
// - The X2 IDs of the destination eNodeBs (as a user can send an InformationElement to several peers)
// - a 'init' flag: if set, the X2 Manager uses the IE and the incoming gate to register the user module  
//                  into its internal data structures. This allows the X2 Manager to know which user is
//                  connected to which gate, so as to forward messages coming from X2 to the correct 
//                  user module.

//
// @class LteX2ControlInfo
//
// This class contains:
// - Source X2 ID
// - List of destionation IDs
// - init flag
//
class X2ControlInfo {
	uint16 sourceId;                          // Source X2NodeId
    DestinationIdList destIdList;             // List of destination X2NodeId
    
    bool init = false;
}