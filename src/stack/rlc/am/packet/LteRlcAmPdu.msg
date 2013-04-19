cplusplus {{

	#include "LteRlcPdu_m.h"

}}

packet LteRlcPdu;
enum LteAmType;


// Rlc Am Control fields for AM control PDUs
//class LteRlcAmControlInfo
//{
// 	@customize(true);
// 	unsigned int firstSn;
// 	unsigned int lastSn;
// 	abstract bool bitmap[];
//}

packet LteRlcAmPdu extends LteRlcPdu
{
    @customize(true);
	unsigned short amType enum(LteAmType);
	unsigned short txNumber;
	int firstSn;
 	int lastSn;
 	abstract bool bitmap[];
}