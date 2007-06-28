#include "performancedb.hxx"

PerformanceDB::PerformanceDB()
{
    // these are the 6 classes originally defined in the PERFSTRUCT
    registerPerformanceData("light", new PerformanceData(
        2.0, 2.0,  450.0, 1000.0,  70.0, 70.0,  80.0, 100.0,  80.0,  70.0, 60.0, 15.0));
    registerPerformanceData("ww2_fighter", new PerformanceData(
        4.0, 2.0,  3000.0, 1500.0,  110.0, 110.0,  180.0, 250.0,  200.0,  130.0, 100.0, 15.0));
    registerPerformanceData("jet_fighter", new PerformanceData(
        7.0, 3.0,  4000.0, 2000.0,  120.0, 150.0,  350.0, 500.0,  350.0,  170.0, 150.0, 15.0));
    registerPerformanceData("jet_transport", new PerformanceData(
        5.0, 2.0,  3000.0, 1500.0,  100.0, 140.0,  300.0, 430.0,  300.0,  170.0, 130.0, 15.0));
    registerPerformanceData("tanker", new PerformanceData(
        5.0, 2.0,  3000.0, 1500.0,  100.0, 140.0,  300.0, 430.0,  300.0,  170.0, 130.0, 15.0));
    registerPerformanceData("ufo", new PerformanceData(
        30.0, 30.0, 6000.0, 6000.0, 150.0, 150.0, 300.0, 430.0, 300.0, 170.0, 130.0, 15.0));

}


PerformanceDB::~PerformanceDB()
{}

void PerformanceDB::registerPerformanceData(const std::string& id, PerformanceData* data) {
    //TODO if key exists already replace data "inplace", i.e. copy to existing PerfData instance
    // this updates all aircraft currently using the PerfData instance.
    _db[id] = data;
}

void PerformanceDB::registerPerformanceData(const std::string& id, const std::string& filename) {
    registerPerformanceData(id, new PerformanceData(filename));
}

PerformanceData* PerformanceDB::getDataFor(const std::string& id) {
    if (_db.find(id) == _db.end()) // id not found -> return jet_transport data
        return _db["jet_transport"];

    return _db[id];
}
