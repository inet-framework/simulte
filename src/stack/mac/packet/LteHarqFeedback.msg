


packet LteHarqFeedback
{
    // H-ARQ acid to which this fb is addressed
    unsigned char acid;
    // H-ARQ cw id to which this fb is addressed
    unsigned char cw;
    // H-ARQ feedback: true for ACK, false for NACK
    bool result;
    // Id of the pdu to which the feedback is addressed
    long pduId;
}
