// 
// This is the MAC message flowing through LTE stack.
//
packet LteMacPdu
{
    @customize(true);
    abstract cPacket sdu[];
    unsigned int headerLength = 0;
    
    //# 
    //# Follows a list of elements only present in
    //# the customized class (see LteMacPdu.h):
    //#
    //# MacSduList sduList;
    //# MacControlElementsList ceList;
    //#
    
}

//
// Mac Control Element
//
class MacControlElement
{
    double timestamp;
}

//
// Mac Buffer Status Report
//
class MacBsr extends MacControlElement 
{
    int size;
}
