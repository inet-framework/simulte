//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
// This is the RLC Data Pdu (base for all RLC modes)
// flowing through LTE stack: basically
// it contains the fields 
// that receiver needs in order to
// correctly decode the original packets
//
packet LteRlcDataPdu
{
    @customize(true);
}
