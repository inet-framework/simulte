//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/phy/das/DasFilter.h"

DasFilter::DasFilter(LtePhyBase* ltePhy, LteBinder* binder,
    RemoteAntennaSet* ruSet, double rssiThreshold)
{
    ruSet_ = ruSet;
    rssiThreshold_ = rssiThreshold;
    binder_ = binder;
    ltePhy_ = ltePhy;
}

DasFilter::~DasFilter()
{
    ruSet_ = NULL;
}

void DasFilter::setMasterRuSet(MacNodeId masterId)
{
    cModule* module = getSimulation()->getModule(binder_->getOmnetId(masterId));
    if (getNodeTypeById(masterId) == ENODEB)
    {
        das_ = check_and_cast<LtePhyEnb*>(module->getSubmodule("lteNic")->
            getSubmodule("phy"))->getDasFilter();
        ruSet_ = das_->getRemoteAntennaSet();
    }
    else
    {
        ruSet_ = NULL;
    }

    // Clear structures used with old master on handover
    reportingSet_.clear();
}

double DasFilter::receiveBroadcast(LteAirFrame* frame, UserControlInfo* lteInfo)
{
    EV << "DAS Filter: Received Broadcast\n";
    EV << "DAS Filter: ReportingSet now contains:\n";
    reportingSet_.clear();

    double rssiEnb = 0;
    for (unsigned int i=0; i<ruSet_->getAntennaSetSize(); i++)
    {
        // equal bitrate mapping
        std::vector<double> rssiV = ltePhy_->getChannelModel()->getSINR(frame,lteInfo);
        std::vector<double>::iterator it;
        double rssi = 0;
        for (it=rssiV.begin();it!=rssiV.end();++it)
            rssi+=*it;
        rssi /= rssiV.size();
        //EV << "Sender Position: (" << senderPos.getX() << "," << senderPos.getY() << ")\n";
        //EV << "My Position: (" << myPos.getX() << "," << myPos.getY() << ")\n";

        EV << "RU" << i << " RSSI: " << rssi;
        if (rssi > rssiThreshold_)
        {
            EV << " is associated";
            reportingSet_.insert((Remote)i);
        }
        EV << "\n";
        if (i == 0)
            rssiEnb = rssi;
    }

    return rssiEnb;
}

RemoteSet DasFilter::getReportingSet()
{
    return reportingSet_;
}

RemoteAntennaSet* DasFilter::getRemoteAntennaSet() const
{
    return ruSet_;
}

double DasFilter::getAntennaTxPower(int i)
{
    return ruSet_->getAntennaTxPower(i);
}

inet::Coord DasFilter::getAntennaCoord(int i)
{
    return ruSet_->getAntennaCoord(i);
}

std::ostream &operator << (std::ostream &stream, const DasFilter* das)
{
    stream << das->getRemoteAntennaSet() << endl;
    return stream;
}
