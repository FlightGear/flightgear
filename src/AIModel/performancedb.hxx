#ifndef PERFORMANCEDB_HXX
#define PERFORMANCEDB_HXX

#include <string>
#include <map>
#include <vector>

class PerformanceData;
class SGPath;

#include <simgear/structure/subsystem_mgr.hxx>

/**
 * Registry for performance data.
 *
 * Allows to store performance data for later reuse/retrieval. Just
 * a simple map for now.
 * 
 * @author Thomas F�rster <t.foerster@biologie.hu-berlin.de>
*/
//TODO provide std::map interface?
class PerformanceDB : public SGSubsystem
{
public:
    PerformanceDB();
    virtual ~PerformanceDB();

    virtual void init();
    virtual void shutdown();

    virtual void update(double dt);

    bool havePerformanceDataForAircraftType(const std::string& acType) const;

    /**
     * get performance data for an aircraft type / class. Type is specific, eg
     * '738' or 'A319'. Class is more generic, such as 'jet_transport'.
     */
    PerformanceData* getDataFor(const std::string& acType, const std::string& acClass) const;

    PerformanceData* getDefaultPerformance() const;

    static const char* subsystemName() { return "aircraft-performance-db"; }
private:
    void load(const SGPath& path);

    void registerPerformanceData(const std::string& id, PerformanceData* data);


    typedef std::map<std::string, PerformanceData*> PerformanceDataDict;
    PerformanceDataDict _db;
    
    const std::string& findAlias(const std::string& acType) const;
  
    typedef std::pair<std::string, std::string> StringPair;
  /// alias list, to allow type/class names to share data. This is used to merge
  /// related types together. Note it's ordered, and not a map since we permit
  /// partial matches when merging - the first matching alias is used.
    std::vector<StringPair> _aliases;
};

#endif
