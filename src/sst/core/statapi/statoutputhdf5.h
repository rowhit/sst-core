// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CORE_STATISTICS_OUTPUTHDF5
#define _H_SST_CORE_STATISTICS_OUTPUTHDF5

#include "sst/core/sst_types.h"

#include <sst/core/statapi/statoutput.h>

#include "H5Cpp.h"
#include <map>
#include <string>

namespace SST {
namespace Statistics {

/**
    \class StatisticOutputHDF5

	The class for statistics output to a comma separated file.
*/
class StatisticOutputHDF5 : public StatisticOutput
{
public:
    /** Construct a StatOutputHDF5
     * @param outputParameters - Parameters used for this Statistic Output
     */
    StatisticOutputHDF5(Params& outputParameters);

    bool acceptsGroups() const { return true; }
protected:
    /** Perform a check of provided parameters
     * @return True if all required parameters and options are acceptable
     */
    bool checkOutputParameters();

    /** Print out usage for this Statistic Output */
    void printUsage();

    void implStartRegisterFields(StatisticBase *stat);
    void implRegisteredField(fieldHandle_t fieldHandle);
    void implStopRegisterFields();

    void implStartRegisterGroup(StatisticGroup* group );
    void implStopRegisterGroup();

    /** Indicate to Statistic Output that simulation started.
     *  Statistic output may perform any startup code here as necessary.
     */
    void startOfSimulation();

    /** Indicate to Statistic Output that simulation ended.
     *  Statistic output may perform any shutdown code here as necessary.
     */
    void endOfSimulation();

    /** Implementation function for the start of output.
     * This will be called by the Statistic Processing Engine to indicate that
     * a Statistic is about to send data to the Statistic Output for processing.
     * @param statistic - Pointer to the statistic object than the output can
     * retrieve data from.
     */
    void implStartOutputEntries(StatisticBase* statistic);

    /** Implementation function for the end of output.
     * This will be called by the Statistic Processing Engine to indicate that
     * a Statistic is finished sendind data to the Statistic Output for processing.
     * The Statisic Output can perform any output related functions here.
     */
    void implStopOutputEntries();

    void implStartOutputGroup(StatisticGroup* group);
    void implStopOutputGroup();

    /** Implementation functions for output.
     * These will be called by the statistic to provide Statistic defined
     * data to be output.
     * @param fieldHandle - The handle to the registered statistic field.
     * @param data - The data related to the registered field to be output.
     */
    void implOutputField(fieldHandle_t fieldHandle, int32_t data);
    void implOutputField(fieldHandle_t fieldHandle, uint32_t data);
    void implOutputField(fieldHandle_t fieldHandle, int64_t data);
    void implOutputField(fieldHandle_t fieldHandle, uint64_t data);
    void implOutputField(fieldHandle_t fieldHandle, float data);
    void implOutputField(fieldHandle_t fieldHandle, double data);

protected:
    StatisticOutputHDF5() {;} // For serialization

private:

    typedef union {
        int32_t     i32;
        uint32_t    u32;
        int64_t     i64;
        uint64_t    u64;
        float       f;
        double      d;
    } StatData_u;


    class DataSet {
    public:
        DataSet(H5::H5File *file) : file(file) { }
        virtual ~DataSet() { }
        virtual bool isGroup() const = 0;

        virtual void setCurrentStatistic(StatisticBase *stat) { }
        virtual void registerField(StatisticFieldInfo *fi) = 0;
        virtual void finalizeCurrentStatistic() = 0;

        virtual void beginGroupRegistration(StatisticGroup *group) { }
        virtual void finalizeGroupRegistration() { }


        virtual void startNewGroupEntry() {}
        virtual void finishGroupEntry() {}

        virtual void startNewEntry() = 0;
        virtual StatData_u& getFieldLoc(fieldHandle_t fieldHandle) = 0;
        virtual void finishEntry() = 0;
    protected:
        H5::H5File *file;
    };

    class StatisticInfo : public DataSet {
        StatisticBase *statistic;
        std::vector<fieldHandle_t> indexMap;
        std::vector<StatData_u> currentData;
        std::vector<fieldType_t> typeList;
        std::vector<std::string> fieldNames;

        H5::DataSet *dataset;
        H5::CompType *memType;

        hsize_t nEntries;

    public:
        StatisticInfo(StatisticBase *stat, H5::H5File *file) :
            DataSet(file), statistic(stat), nEntries(0)
        {
            typeList.push_back(StatisticFieldInfo::UINT64);
            indexMap.push_back(-1);
        }
        ~StatisticInfo() {
            if ( dataset ) delete dataset;
            if ( memType ) delete memType;
        }
        void registerField(StatisticFieldInfo *fi);
        void finalizeCurrentStatistic();

        bool isGroup() const { return false; }
        void startNewEntry();
        StatData_u& getFieldLoc(fieldHandle_t fieldHandle);
        void finishEntry();
    };

    class GroupInfo : public DataSet {
    public:
        GroupInfo(StatisticGroup *group, H5::H5File *file) : DataSet(file) { /* TODO */ }
        void beginGroupRegistration(StatisticGroup *group) { }
        void setCurrentStatistic(StatisticBase *stat) { /* TODO */ }
        void registerField(StatisticFieldInfo *fi) { /* TODO */ }
        void finalizeCurrentStatistic() { /* TODO */ }
        void finalizeGroupRegistration() { /* TODO */ }

        bool isGroup() const { return true; }
        void startNewEntry() { /* TODO */ }
        StatData_u& getFieldLoc(fieldHandle_t fieldHandle) { /* TODO */ }
        void finishEntry() { /* TODO */ }

        void startNewGroupEntry() { /* TODO */ }
        void finishGroupEntry() { /* TODO */ }
    };


    H5::H5File*              m_hFile;
    DataSet*                 m_currentDataSet;
    StatisticInfo*           m_currentStatistic;
    std::map<StatisticBase*, StatisticInfo*> m_statistics;
    std::map<std::string, GroupInfo> m_statGroups;


    StatisticInfo*  initStatistic(StatisticBase* statistic);
    StatisticInfo*  getStatisticInfo(StatisticBase* statistic);
};

} //namespace Statistics
} //namespace SST

#endif
