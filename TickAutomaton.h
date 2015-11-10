/*
 * TickAutomaton.h
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#ifndef TICKAUTOMATON_H_
#define TICKAUTOMATON_H_

#include "StateMachine.h"

#include <string>
#include <vector>

namespace inet {

using std::string;
using std::vector;

enum TickAutomatonTypes {
    ACTIVATE = 3,
    TIME_OUT = 4
};

/**
 *
 */
class ITimeOut {
public:
    virtual void timeOut() = 0;
};

class ITimeOutProducer {
protected:
    class Record {
    public:
        double d;
        ITimeOut* listener;
    };
    vector< Record > listeners;
public:
    virtual void registerListener(ITimeOut* listener, double afterElapsedTime);
};

StateMachine* buildTicker(string name, double d, StateMachine* target, MessageType msgId, ITimeOutProducer* top);

}


#endif /* TICKAUTOMATON_H_ */
