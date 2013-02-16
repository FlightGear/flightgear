#ifndef PERFORMANCEDB_HXX
#define PERFORMANCEDB_HXX

#include <string>
#include <map>
#include <vector>

class PerformanceData;
class SGPath;

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

    /**
     * get performance data for an aircraft type / class. Type is specific, eg
     * '738' or 'A319'. Class is more generic, such as 'jet_transport'.
     */
    PerformanceData* getDataFor(const std::string& acType, const std::string& acClass);
    void load(const SGPath& path);

private:
    std::map<std::string, PerformanceData*> _db;
    
    const std::string& findAlias(const std::string& acType) const;
  
    typedef std::pair<std::string, std::string> StringPair;
  /// alias list, to allow type/class names to share data. This is used to merge
  /// related types together. Note it's ordered, and not a map since we permit
  /// partial matches when merging - the first matching alias is used.
    std::vector<StringPair> _aliases;
};

#endif
