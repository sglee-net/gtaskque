//
// Created by SG Lee on 2019/11/02.
//

#ifndef GTASKQUE_EXECUTORTHROW_H
#define GTASKQUE_EXECUTORTHROW_H

#include <future>
#include <thread>
#include "Attribute.h"
#include "gtaskque.h"

using namespace std;

// template <typename T, typename E>
// T: type of data with which an executor handles
// E: data type of executor or attribute
template <typename T, typename E>
class ExecutorThrow: public GExecutorInterface<T,E> {
public:
    ExecutorThrow(E *attribute, bool b)
            :GExecutorInterface<T,E>(attribute, b) {}
    virtual ~ExecutorThrow() {
    }
    int execute(T &arg) = 0;
};

template <>
class ExecutorThrow<string, Attribute>
        : public GExecutorInterface<string, Attribute> {
public:
    ExecutorThrow(Attribute *attribute, bool b)
            :GExecutorInterface<string, Attribute>(attribute, b) {}
    virtual ~ExecutorThrow() {
    }
    int execute(string &arg) const {
        usleep(100000);
        throw runtime_error("test throw");
        return 0;
    }
};

#endif //GTASKQUE_EXECUTORTHROW_H
