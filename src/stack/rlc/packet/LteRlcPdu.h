//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef LTERLCPDU_H_
#define LTERLCPDU_H_

#include "LteRlcPdu_m.h"
#include "LteControlInfo.h"

class LteRlcPdu : public LteRlcPdu_Base {
   private:
     void copy(const LteRlcPdu& other)
     {
         this->totalFragments_var = other.totalFragments_var;
         this->snoFragment_var = other.snoFragment_var;
         this->snoMainPacket_var = other.snoMainPacket_var;
         // copy the attached control info, if any
         if (other.getControlInfo() != NULL)
         {
             if (this->getControlInfo() != NULL)
             {
                 FlowControlInfo* info = check_and_cast<FlowControlInfo*>(this->removeControlInfo());
                 delete info;
             }

             FlowControlInfo* info = check_and_cast<FlowControlInfo*>(other.getControlInfo());
             FlowControlInfo* info_dup = info->dup();
             this->setControlInfo(info_dup);
         }
     }

   public:
     LteRlcPdu(const char *name=NULL, int kind=0) : LteRlcPdu_Base(name,kind) {}
     LteRlcPdu(const LteRlcPdu& other) : LteRlcPdu_Base(other) {copy(other);}

     LteRlcPdu& operator=(const LteRlcPdu& other)
     {
         if (this==&other)
             return *this;
         LteRlcPdu_Base::operator=(other);
         copy(other);
         return *this;
     }
     virtual LteRlcPdu *dup() const {return new LteRlcPdu(*this);}
};

Register_Class(LteRlcPdu);

#endif /* LTERLCPDU_H_ */
