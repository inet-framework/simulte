//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_X2HANDOVERCOMMAND_H_
#define _LTE_X2HANDOVERCOMMAND_H_

#include "x2/packet/X2InformationElement.h"

//
// X2HandoverCommandIE
//
class X2HandoverCommandIE : public X2InformationElement
{
protected:

    bool startHandover_;
    MacNodeId ueId_;      // ID of the user performing handover

public:
  X2HandoverCommandIE()
  {
      type_ = X2_HANDOVER_CMD_IE;
      length_ = sizeof(MacNodeId);
      startHandover_ = false;
  }
  X2HandoverCommandIE(const X2HandoverCommandIE& other) :
      X2InformationElement()
  {
      operator=(other);
  }

  X2HandoverCommandIE& operator=(const X2HandoverCommandIE& other)
  {
      if (&other == this)
          return *this;
      X2InformationElement::operator=(other);
      return *this;
  }
  virtual X2HandoverCommandIE *dup() const
  {
      return new X2HandoverCommandIE(*this);
  }
  virtual ~X2HandoverCommandIE() {}

  // getter/setter methods
  void setStartHandover() { startHandover_ = true; }
  bool isStartHandover() { return startHandover_; }
  void setUeId(MacNodeId ueId) { ueId_ = ueId; }
  MacNodeId getUeId() { return ueId_; }
};

#endif
