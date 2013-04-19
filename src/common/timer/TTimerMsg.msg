

cplusplus {{
	#include "LteCommon.h" 
}};

enum TTimerType;

packet TTimerMsg 
{	
    unsigned int timerId;
    unsigned short type enum(TTimerType);
}
