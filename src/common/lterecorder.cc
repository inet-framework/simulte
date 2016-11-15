//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
//

#include "lterecorder.h"
#include "LteBinder.h"

/*
 * Register recorders
 */

Register_ResultRecorder("lteHistogram", LteHistogramRecorder);
Register_ResultRecorder("lteStats", LteStatsRecorder);
Register_ResultRecorder("lteVector", LteVectorRecorder);
Register_ResultRecorder("lteAvg", LteAvgRecorder);
Register_ResultRecorder("lteRate", LteRateRecorder);

/*
 * LteStatisticRecorder member functions
 */

LteStatisticsRecorder::~LteStatisticsRecorder()
{
    // Delete all stored cStatistic objects
    std::map<unsigned int, cStatistic*>::iterator it;
    for (it = stats_.begin(); it != stats_.end(); it++)
        delete it->second;
    stats_.clear();
}

void LteStatisticsRecorder::finish(cResultFilter *prev)
{
    opp_string_map attributes = getStatisticAttributes();
    //char metricName[50];
    std::map<unsigned int, cStatistic*>::iterator it;
    for (it = stats_.begin(); it != stats_.end(); it++)
    {
        // UE might have left the simulation, in this case,
        // finish has already been called
        int id = getBinder()->getOmnetId(it->first);
        if(id == 0){
                continue;
        }
        // Record metrics for all IDs
        getEnvir()->recordStatistic(moduleMap_[it->first], /*metricName*/ getResultName().c_str(), it->second, &attributes);
    }
}

/*
 * LteStatsRecorder member functions
 */

void LteStatsRecorder::subscribedTo(cResultFilter *prev)
{
}

void LteStatsRecorder::collect(simtime_t t, double value, unsigned int id, cComponent* module)
{
    if (!stats_[id])
        stats_[id] = new cStdDev();
    stats_[id]->collect(value);    // Local Recording

    moduleMap_[id] = module;
}

/*
 * LteHistogramRecorder member functions
 */

void LteHistogramRecorder::subscribedTo(cResultFilter *prev)
{
    stats_[0] = new cHistogram();
}

void LteHistogramRecorder::collect(simtime_t t, double value, unsigned int id, cComponent* module)
{
    if (!stats_[id])
        stats_[id] = new cHistogram();
    stats_[id]->collect(value);    // Local Recording
    moduleMap_[id] = module;
}

/*
 * LteVectorRecorder member functions
 */

void LteVectorRecorder::subscribedTo(cResultFilter *prev)
{
    cNumericResultRecorder::subscribedTo(prev);

    // we can register the vector here, because base class ensures we are subscribed only at once place
    opp_string_map attributes = getStatisticAttributes();

    // register global vector handle
    handle_[0] = getEnvir()->registerOutputVector(getComponent()->getFullPath().c_str(), getResultName().c_str());
    ASSERT(handle_[0] != NULL);
    for (opp_string_map::iterator it = attributes.begin(); it != attributes.end(); ++it)
        getEnvir()->setVectorAttribute(handle_[0], it->first.c_str(), it->second.c_str());
    }

void LteVectorRecorder::collect(simtime_t t, double value, unsigned int id, cComponent* module)
{
    if (t < lastTime_)
    {
        throw cRuntimeError("%s: Cannot record data with an earlier timestamp (t=%s) "
            "than the previously recorded value (t=%s)",
            cResultListener::getClassName(), SIMTIME_STR(t), SIMTIME_STR(lastTime_));
    }

    moduleMap_[id] = module;

    lastTime_ = t;
    if (!handle_[id])
    {
        // register vector handle for new id
        opp_string_map attributes = getStatisticAttributes();
        char metricName[50];
        sprintf(metricName, "%s:id=%d", getResultName().c_str(), id);

        handle_[id] = getEnvir()->registerOutputVector(moduleMap_[id]->getFullPath().c_str(), metricName);
        ASSERT(handle_[id] != NULL);
        for (opp_string_map::iterator it = attributes.begin(); it != attributes.end(); ++it)
            getEnvir()->setVectorAttribute(handle_[id], it->first.c_str(), it->second.c_str());
        }

    getEnvir()->recordInOutputVector(handle_[id], t, value);        // Local Recording
}

/*
 * LteAvgRecorder member functions
 */

void LteAvgRecorder::collect(simtime_t t, double value, unsigned int id, cComponent* module)
{
    vals_[id].count_++;
    vals_[id].sum_ += value;
    moduleMap_[id] = module;
}

void LteAvgRecorder::finish(cResultFilter *prev)
{
    opp_string_map attributes = getStatisticAttributes();
    //char metricName[50];
    double totalSum = 0;        // Global numbers
    std::map<unsigned int, recordedValues_>::iterator it;
    for (it = vals_.begin(); it != vals_.end(); it++)
    {
        // Record metrics for all IDs
        int id = getBinder()->getOmnetId(it->first);
        if(id == 0){
                // UE had left the simulation before
                continue;
        }
        totalSum += (it->second.sum_ / it->second.count_);
        getEnvir()->recordScalar(moduleMap_[it->first], getResultName().c_str(),
            it->second.sum_/it->second.count_, &attributes);
    }
}

/*
 * LteRateRecorder member functions
 */

void LteRateRecorder::collect(simtime_t t, double value, unsigned int id, cComponent* module)
{
    if (vals_[id].startTime_ == 0)
    {
        vals_[id].startTime_ = t;
    }
    vals_[id].sum_ += value;

    moduleMap_[id] = module;
}

void LteRateRecorder::finish(cResultFilter *prev)
{
    opp_string_map attributes = getStatisticAttributes();
    double interval, totalSum = 0;        // Global numbers

    std::map<unsigned int, recordedValues_>::iterator it;
    for (it = vals_.begin(); it != vals_.end(); it++)
    {
        interval = (simTime() - it->second.startTime_).dbl();
        totalSum += it->second.sum_ / interval;

        int id = getBinder()->getOmnetId(it->first);
        if(id == 0){
                // UE had left the simulation before - skip it
                continue;
        }
        getEnvir()->recordScalar(moduleMap_[it->first], getResultName().c_str(),
            it->second.sum_/interval, &attributes);
    }
}

