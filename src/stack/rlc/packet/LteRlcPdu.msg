
cplusplus {{
    #include "LteRlcDefs.h"
}};

// 
// This is the RLC Pdu
// flowing through LTE stack: basically
// it contains the fields of a fragment
// that receiver needs in order to
// correctly decode the original packet
//
packet LteRlcPdu
{
    unsigned int totalFragments;	// Total number of fragments
    unsigned int snoFragment;		// Sequence number of this fragment
    unsigned int snoMainPacket;		// ID of packet (sequence number)
}
