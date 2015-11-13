/*
 * StateMachineInterpreter.cc
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#include "StateMachineInterpreter.h"

#include <iostream>

namespace inet {

StateMachineInterpreter::~StateMachineInterpreter() {
    // TODO Auto-generated destructor stub
}

bool StateMachineInterpreter::move()
{
    MessagePool* p = sm->getPool();
    bool f;
//    std::cout << "Pool " << sm->getName() <<  " contains : " << p->count() << " elements " << std::endl;
    int c = 0;
    do {
        f = false;
        int i = 0;
        for (i = 0 ; i < p->count() ; i++) {

            f = current->existsTransition((*p)[i]);
//            std::cout << "in move : " << (*p)[i] << " and results in " << f << std::endl;
            if (f) {
                break;
            }
        }
        if (f) {
            c++;
            MessageType m = (*p)[i];
//            std::cout << " going next " << std::endl;
            current = current->next(m);
//            std::cout << " Now it is Ok : " << current->getName() << std::endl;
            current->getActions()->enteringState(current, sm, m, p->getExtraData(m));
            p->drop(i);
        }
    } while (f);

    return c > 0;
}

} /* namespace inet */
