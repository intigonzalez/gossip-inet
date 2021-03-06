/*
 * StateMachine.h
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <map>

using std::vector;
using std::map;
using std::string;
using std::runtime_error;

namespace inet {


typedef int MessageType;

enum BasicTypes {
    MSG_TRUE = 1,
    MSG_FALSE = 2
};

class StateMachine;
class State;
class MessagePool;

/**
 * A state Machine, isn't it obvious? :-P
 */
class StateMachine {
protected:
    vector<State*> states;
    int initialState = 0;
    MessagePool* pool;
    string name;
public:
    StateMachine(string n);
    virtual ~StateMachine();

    bool addState(State* s);

    bool addTransition(MessageType id, State* from, State* to);

    State* getInitialState() { return states[initialState]; }
    void setInitialState(State* s);

    State* getState(int idx);

    virtual void reportMessage(MessageType msgId);

    MessagePool* getPool();

    string getName() { return name; }
};

/**
 *
 */
class Transition {
protected:
    MessageType msgID;

    int to;
public:
    Transition(MessageType id, int t):msgID(id), to(t) {};
    virtual ~Transition();

    MessageType getMessageId() const { return msgID; }
    int getTo() const { return to; }
};

class StateActions {
public:
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) = 0;
};

class NoActions : public StateActions {
public:
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {}
};

class LogActions : public StateActions {
private:
    std::string msg;
public:
    LogActions(std::string a): msg(a) {}

    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        std::cout << " LALLAAL  " << this->msg << std::endl;
    }
};

/**
 *
 */
class State {
protected:
    string name;
    vector<Transition*> transitions;
    StateMachine* owner = nullptr;
    StateActions* actions = nullptr;
public:

    State(string n, StateActions* a):name(n),actions(a) {}
    State(const State& other);

    void addTransition(MessageType id, int to);

    bool existsTransition(MessageType id);

    State* next(MessageType id);

    string getName() {  return name; }

    StateActions* getActions()  { return actions; }

    bool operator==(const State& other);

    friend bool StateMachine::addState(State* s);
};

/**
 * A pool of messages, notice that is not a queue
 * FIXME: I should be sent to jail for this implementation
 */
class MessagePool {
protected:
    vector<MessageType> messages;
    map<int, void*> extraData;
public:
    void add(MessageType msg) { messages.push_back(msg); }
    void add(MessageType msg, void* extraData);
    bool isEmpty() {  return messages.size() == 0;  }
    int count() { return messages.size(); }
    MessageType operator[] (int idx) { return messages[idx]; }
    void* getExtraData(MessageType m);
    MessageType drop(int idx);
};

} /* namespace inet */

#endif /* STATEMACHINE_H_ */
