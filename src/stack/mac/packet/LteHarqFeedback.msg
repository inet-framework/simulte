//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

packet LteHarqFeedback
{
    // H-ARQ acid to which this fb is addressed
    unsigned char acid;
    // H-ARQ cw id to which this fb is addressed
    unsigned char cw;
    // H-ARQ feedback: true for ACK, false for NACK
    bool result;
    // Id of the pdu to which the feedback is addressed
    long fbMacPduId;
    // if this flag is true, then the feedback refers to a D2D connection
    bool d2dFeedback = false;
}

packet LteHarqFeedbackMirror extends LteHarqFeedback
{
    d2dFeedback = true;
    
    // Id of the D2D Transmitter
    long d2dSenderId;
    // Id of the D2D Receiver
    long d2dReceiverId;   
    // Length (in bytes) of the corresponding PDU
    long pduLength;
}
