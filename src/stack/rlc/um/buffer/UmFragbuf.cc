//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/um/buffer/UmFragbuf.h"

bool UmFragbuf::insert(unsigned int pktId, unsigned int totFrag,
    unsigned int fragSno, unsigned int fragSize, FlowControlInfo* info)
{
    debugCounter_++;
    //EV << "debugCounter is now " << debugCounter_ << endl;

    // Add fragment to buffer
    if (frags_.find(pktId) == frags_.end())
    {
        // No fragments for this Main packet
        MainPktInfo_ pkt;
        pkt.fragsleft_ = totFrag - 1;
        pkt.size_ = fragSize;
        pkt.bitmap_ = std::vector<bool>(totFrag, false);        // Insert a new Main packet vector
        pkt.lteInfo_ = info;
        frags_.insert(std::pair<unsigned int, MainPktInfo_>(pktId, pkt));
        frags_[pktId].bitmap_[fragSno] = true;
        return true;
    }
    else if (frags_[pktId].bitmap_[fragSno] == false)
    {
        // Set this fragment if not a duplicate fragment
        frags_[pktId].fragsleft_--;
        frags_[pktId].size_ += fragSize;
        frags_[pktId].bitmap_[fragSno] = true;
        return false;
    }
    else
    {
        // FIXME In this case, we receive a duplicate fragment, is it ok to abort??
        throw cRuntimeError("UmFragbuf::insert(): inconsistency detected with packet %d", pktId);
    }
}

bool UmFragbuf::check(unsigned int pktID)
{
    if (frags_[pktID].fragsleft_ == 0)
    {
        // All fragments received
        return true;
    }
    else
    {
        return false;
    }
}

int UmFragbuf::remove(unsigned int pktId)
{
    int mainPktSize = frags_[pktId].size_;
    delete frags_[pktId].lteInfo_;
    frags_.erase(pktId);    // Remove Fragments
    return mainPktSize;
}

FlowControlInfo* UmFragbuf::getLteInfo(unsigned int pktId)
{
    return frags_.at(pktId).lteInfo_;
}

std::ostream &operator << (std::ostream &stream, const UmFragbuf* fragbuf)
{
    if (fragbuf == NULL)
        return (stream << "Empty set");
    std::map<unsigned int, UmFragbuf::MainPktInfo_>::iterator it;
    std::map<unsigned int, UmFragbuf::MainPktInfo_> frags = fragbuf->frags_;
    for (it = frags.begin(); it != frags.end(); it++)
    {
        stream << "Packet: " << it->first << " received fragments: ";
        for (unsigned int i = 0; i < it->second.bitmap_.size(); i++)
        {
            if (it->second.bitmap_[i] == false)
                stream << "0";
            else
                stream << "1";
        }
        stream << ";  ";
    }
    return stream;
}
