#ifndef PERFORMANCEDB_HXX
#define PERFORMANCEDB_HXX

#include <string>
#include <vector>
#include <map>

#include "performancedata.hxx"

/**
 * Registry for performance data.
 *
 * Allows to store performance data for later reuse/retrieval. Just
 * a simple map for now.
 * 
 * @author Thomas F�rster <t.foerster@biologie.hu-berlin.de>
*/
//TODO provide std::map interface?
class PerformanceDB
{
public:
    PerformanceDB();
    ~PerformanceDB();

    void registerPerformanceData(const std::string& id, PerformanceData* data);
    void registerPerformanceData(const std::string& id, const std::string& filename);

    PerformanceData* getDataFor(const std::string& id);
    void load(SGPath path);

private:
    std::map<std::string, PerformanceData*> _db;
};

#endif
