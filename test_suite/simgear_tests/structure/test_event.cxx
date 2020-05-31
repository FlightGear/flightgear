/*
 * Copyright (C) 2016 Edward d'Auvergne
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "test_event.hxx"

#include <algorithm>

using namespace std;

namespace {

SGEventMgr* global_eventManager = nullptr;
double global_realTime = 0.0;

// class which behaves similarly to TimerObj in NasalSys
class FakeNasalTimer
{
public:
    FakeNasalTimer(string n) : _name(n)
    {
    }

    void invoke()
    {
        ++_invokeCount;

        if (_expectedTime > 0.0) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(_expectedTime, global_realTime, 0.1);
        }

        // rescheudle base don the next time
        if (!_rescheduleTimes.empty()) {
            const auto t = _rescheduleTimes.front();
            _rescheduleTimes.pop_front();

            // TimerObj does this, eugh
            global_eventManager->removeTask(_name);
            start(t);
        }
    }

    void start(double interval)
    {
        global_eventManager->addTask(_name, this, &FakeNasalTimer::invoke,
                                     interval, interval /* delay */, false);

        _expectedTime = global_realTime + interval;
    }

    string _name;
    deque<double> _rescheduleTimes;
    int _invokeCount = 0;
    double _expectedTime = 0.0;
};

} // namespace

// Set up function for each test.
void SimgearEventTests::setUp()
{
    _eventManager = new SGEventMgr;
    global_eventManager = _eventManager;
    _eventManager->init();

    _realTimeProp = new SGPropertyNode{};
    _eventManager->setRealtimeProperty(_realTimeProp);
}


// Clean up after each test.
void SimgearEventTests::tearDown()
{
    global_eventManager = nullptr;
}

void SimgearEventTests::runForTestTime(double totalTime, double updateHz, double simTimeScaling)
{
    const double dt = 1.0 / updateHz;
    const int updateCount = static_cast<int>(totalTime * updateHz);
    const double simDt = dt * simTimeScaling;
    _realTimeProp->setDoubleValue(dt);
    global_realTime = 0.0;

    for (int c = 0; c < updateCount; ++c) {
        const double newRealTime = global_realTime + dt;
        global_realTime = newRealTime;
        _eventManager->update(simDt);
    }
}

void SimgearEventTests::testTaskRescheduleDuringRun()
{
    auto t1 = new FakeNasalTimer("timer_aaa");
    t1->_rescheduleTimes.resize(10);
    fill(t1->_rescheduleTimes.begin(), t1->_rescheduleTimes.end(), 0.5);
    t1->_rescheduleTimes.push_back(30.0); // large final value

    global_realTime = 0.0;
    t1->start(1.0);
    runForTestTime(8.0, 20);

    CPPUNIT_ASSERT_EQUAL(11, t1->_invokeCount);
}
