/*
 * StateMachineInterpreter.h
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#ifndef STATEMACHINEINTERPRETER_H_
#define STATEMACHINEINTERPRETER_H_

#include "StateMachine.h"

namespace inet {

class StateMachineInterpreter {
protected:
    StateMachine* sm;
    State* current;
public:
    StateMachineInterpreter(StateMachine* sm):sm(sm), current(sm->getInitialState()) {}
    virtual ~StateMachineInterpreter();

    bool move();
};

} /* namespace inet */

#endif /* STATEMACHINEINTERPRETER_H_ */
