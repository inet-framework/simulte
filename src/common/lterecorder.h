//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERECORDERS_H_
#define _LTE_LTERECORDERS_H_

#include <omnetpp.h>
#include <map>

using namespace omnetpp;

/**
 * \class TaggedSample
 * \brief Object used to send samples with IDs
 *
 * This class is used so that the sender can pass
 * a cObject containing both the sample and the id
 */
class TaggedSample : public cObject
{
  public:
    double sample_;
    unsigned int id_;
    // the emitting cComponent (module)
    cComponent* module_;
};

/**
 * \class LteRecorder
 * \brief Abstract class for recording statistic
 *
 * This class is the lteRecorder. It records samples and has
 * an optional parameter "id" specified by the caller, that can
 * be used to gather separate statistics
 */
class LteRecorder : public cNumericResultRecorder, private cObject
{
  protected:

    // Map associating each metric ID with its emitting module
    std::map<unsigned int, cComponent*> moduleMap_;

  public:
    LteRecorder()
    {
    }
    virtual ~LteRecorder()
    {
    }

    /**
     * finish() is used to store statistics (when scalar)
     *
     * @param prev Filter used for this signal
     */
    virtual void finish(cResultFilter *prev)
    {
    }

    /**
     * subscribeTo() is called when a module subscribes to a
     * recorder and is used to initialize the global recorders
     * (the ones independent from submitted id)
     *
     * @param prev Filter used for this signal
     */
    virtual void subscribedTo(cResultFilter *prev)
    {
    }

    void deleteModule(unsigned int nodeId){
        moduleMap_.erase(nodeId);
    }

  protected:

    /**
     * collect() is virtual in NumericResultRecorder, does nothing
     *
     * @param t Reference to simulation time event occurred
     * @param value Sample received
     */
    virtual void collect(const simtime_t& t, double value, cObject* details)
    {
    }

    /**
     * collect() function must be redefined by subclasses, it collects
     * statistics and has an optional "id" parameter
     *
     * @param t Reference to simulation time event occurred
     * @param value Sample received
     * @param id Id specified by the caller
     */
    virtual void collect(simtime_t t, double value, unsigned int id = 0, cComponent* module = NULL) = 0;

    /**
     * receiveSignal() function receives the samples emitted by the caller
     * and extract values needed by the collect() function
     *
     * @param prev Filter used for this signal
     * @param t Time event occurred
     * @param id obj TaggedSample, contains the sample and the id
     */
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *)
    {
        collect(t, ((TaggedSample*) object)->sample_, ((TaggedSample*) object)->id_, ((TaggedSample*) object)->module_);
    }
};

/**
 * \class LteStatisticsRecorder
 * \brief Abstract class for recording cStatistic objects
 * This class records object of type cStatistic aggregating them
 * by name of the module and by "id" specified by the caller,
 * moreover there is a global statistic (without id)
 */
class LteStatisticsRecorder : public LteRecorder
{
  public:
    LteStatisticsRecorder()
    {
    }

    /**
     * Destroys all elements allocated
     * inside the above map
     */
    virtual ~LteStatisticsRecorder();

    /**
     * finish() records to an output file scalar metrics
     * of type cStatistic grouping them by:
     * - Module who gathered the statistic
     * - Id specified by the module
     *
     * Moreover an aggregate metric for all IDs of
     * every module is recorded aswell
     */
    virtual void finish(cResultFilter *prev);

    virtual void subscribedTo(cResultFilter *prev)
    {
    }

  protected:
    /**
     * Map associating each id with a
     * cStatistic object
     */
    std::map<unsigned int, cStatistic*> stats_;
};

/**
 * \class LteStatsRecorder
 * \brief Implements LteStatisticRecorder using cStdDev objects
 *
 * This class implements the LteStatisticRecorder using
 * objects of type cStdDev
 */
class LteStatsRecorder : public LteStatisticsRecorder
{
  public:
    LteStatsRecorder()
    {
    }
    virtual ~LteStatsRecorder()
    {
    }

    /**
     * subscibeTo() creates a new cHistogram
     * inside stats_[0]
     *
     * @param prev Filter used for this signal
     */
    virtual void subscribedTo(cResultFilter *prev);

  protected:
    /**
     * collect() collects sample inside a different
     * cStdDev object for each id specified: if the cStdDev does
     * not exist yet for this id, it is created
     *
     * @param t Reference to simulation time event occurred
     * @param value Sample received
     * @param id Id specified by the caller
     */
    virtual void collect(simtime_t t, double value, unsigned int id, cComponent* module);
};

/**
 * \class LteHistogramRecorder
 * \brief Implements LteStatisticRecorder using cHistogram objects
 *
 * This class implements the LteStatisticRecorder using
 * objects of type cHistogram
 */
class LteHistogramRecorder : public LteStatisticsRecorder
{
  public:
    LteHistogramRecorder()
    {
    }
    virtual ~LteHistogramRecorder()
    {
    }

    /**
     * subscibeTo() creates a new cHistogram
     * inside stats_[0]
     *
     * @param prev Filter used for this signal
     */
    virtual void subscribedTo(cResultFilter *prev);

  protected:
    /**
     * collect() collects sample inside a different
     * cHistogram object for each id specified: if the cHistogram
     * does not exist yet for this id, it is created
     *
     * @param t Reference to simulation time event occurred
     * @param value Sample received
     * @param id Id specified by the caller
     */
    virtual void collect(simtime_t t, double value, unsigned int id, cComponent* module);
};

/**
 * \class LteVectorRecorder
 * \brief Records samples associating a timestamp to each one
 *
 * This class uses the cOutVector (implicitly) to store
 * samples associating a timestamp to each of them
 */
class LteVectorRecorder : public LteRecorder
{
  public:
    LteVectorRecorder()
    {
        lastTime_ = 0;
    }
    virtual ~LteVectorRecorder()
    {
        handle_.clear();
    }

    /**
     * subscibeTo() registers a new output
     * vector inside handle_[0]
     *
     * @param prev Filter used for this signal
     */
    virtual void subscribedTo(cResultFilter *prev);

  protected:
    /**
     * collect() performs the following tasks:
     * - Verify that the samples ordering is correct
     * - If there is not an handle for current is, registers
     *   a new output vector and associates the handle to it
     * - Record the sample to the output vector for this id
     *   and for the "global" statistic
     *
     * @param t Reference to simulation time event occurred
     * @param value Sample received
     * @param id Id specified by the caller
     */
    virtual void collect(simtime_t t, double value, unsigned int id, cComponent* module);

  private:
    /// This variable is used to ensure increasing timestamp order
    simtime_t lastTime_;

    /**
     * This map associated each id with an handle
     * who identifies the output vector for the
     * output vector manager
     */
    std::map<unsigned int, void*> handle_;
};

/**
 * \class LteAvgRecorder
 * \brief Average metrics recoder
 *
 * This class implements the LteRecorder and records
 * average values for the given samples, dividing them
 * by id (an aggragate metric is also given)
 *
 * Usage example: to record delay values just issue
 * emit() inside the module, providing the delay
 * value and an id (delay of flow $i).
 * Average between all emitted values for all IDs
 * will be automatically calculated
 */
class LteAvgRecorder : public LteRecorder
{
  private:
    /**
     * \struct recordedValues_
     * \brief store samples
     *
     * Samples are stored inside this structure by
     * incrementing count every time and adding
     * the sample to the total sum
     */
    struct recordedValues_
    {
        unsigned int count_;
        double sum_;
    };

    /**
     * Map associating each id with a
     * recorded values structure
     */
    std::map<unsigned int, recordedValues_> vals_;

  protected:
    /**
     * collect() collects sample inside a different
     * recordedValues_ structure for each id specified
     *
     * @param t Reference to simulation time event occurred
     * @param value Sample received
     * @param id Id specified by the caller
     */
    virtual void collect(simtime_t t, double value, unsigned int id, cComponent* module);

  public:
    LteAvgRecorder()
    {
    }
    virtual ~LteAvgRecorder()
    {
        vals_.clear();
    }

    /**
     * finish() records to an output file the scalar
     * metric recorded. Values are grouped by:
     * - Module who gathered the statistic
     * - Id specified by the module
     *
     * Moreover an aggregate metric for all IDs of
     * every module is recorded aswell
     */
    virtual void finish(cResultFilter *prev);
};

/**
 * \class LteRateRecorder
 * \brief Rate metrics recoder
 *
 * This class implements the LteRecorder and records
 * rate values for the given samples, dividing them
 * by id (an aggragate metric is also given)
 *
 * Usage example: to record throughput values:
 * - issue an emit on the sender module providing "0" as
 *   sample and a correct id (so the startTime is initialized)
 * - issue different emits on the receiver module specifying
 *   the size of the received packet.
 * Throughput will be automatically calculated dividing the
 * total received size by the transmission interval time
 *
 */
class LteRateRecorder : public LteRecorder
{
  protected:
    /**
     * \struct recordedValues_
     * \brief store samples
     *
     * Samples are stored inside this structure:
     * startTime is initialized by the sender and
     * sum is incremented by the receiver
     */
    struct recordedValues_
    {
        simtime_t startTime_;
        double sum_;

    public:
        recordedValues_()
        {
            startTime_ = 0;
            sum_ = 0;
        }
    };

    /**
     * Map associating each id with a
     * recorded values structure
     */
    std::map<unsigned int, recordedValues_> vals_;

  protected:
    /**
     * collect() collects sample inside a different
     * recordedValues_ structure for each id specified
     *
     * @param t Reference to simulation time event occurred
     * @param value Sample received
     * @param id Id specified by the caller
     */
    virtual void collect(simtime_t t, double value, unsigned int id, cComponent* module);

  public:
    LteRateRecorder()
    {
    }
    virtual ~LteRateRecorder()
    {
        vals_.clear();
    }

    /**
     * finish() records to an output file the scalar
     * metric recorded. Values are grouped by:
     * - Module who gathered the statistic
     * - Id specified by the module
     *
     * Moreover an aggregate metric for all IDs of
     * every module is recorded aswell
     */
    virtual void finish(cResultFilter *prev);
};

#endif

