/*
 * StateMachineInterpreter.cc
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#include "StateMachineInterpreter.h"

namespace inet {

StateMachineInterpreter::~StateMachineInterpreter() {
    // TODO Auto-generated destructor stub
}

void StateMachineInterpreter::move()
{
    MessagePool* p = sm->getPool();
    bool f = false;
    do {
        f = false;
        int i = 0;
        for (i = 0 ; i < p->count() ; i++) {
            if (current.existsTransition((*p)[i])) {
                f = true;
                break;
            }
        }
        if (f) {
            MessageType m = (*p)[i];
            current = current.next(m);
            current.getActions()->enteringState(current, sm);
            p->drop(i);
        }
    } while (!f);
}

} /* namespace inet */
