//
// Created by SG Lee on 2019/10/25.
//

#ifndef SEGMENT_MEASUREMENT_SEARCH_EXECUTORHTTPPOST_H
#define SEGMENT_MEASUREMENT_SEARCH_EXECUTORHTTPPOST_H

#include <future>
#include <thread>
#include "Attribute.h"
#include "gtaskque.h"

using namespace std;

// template <typename T, typename E>
// T: type of data with which an executor handles
// E: data type of executor or attribute
template <typename T, typename E>
class ExecutorStream: public GExecutorInterface<T,E> {
public:
    ExecutorStream(E *attribute, bool b)
            :GExecutorInterface<T,E>(attribute, b) {}
    virtual ~ExecutorStream() {
    }
    int execute(T &arg) = 0;
};

template <>
class ExecutorStream<string, Attribute>
        : public GExecutorInterface<string, Attribute> {
public:
    ExecutorStream(Attribute *attribute, bool b)
        :GExecutorInterface<string, Attribute>(attribute, b) {}
    virtual ~ExecutorStream() {
    }
    int execute(string &arg) const {
        const Attribute *attribute = this->getAttribute();

        cout << arg << endl;

        return 0;
    }
};

#endif //SEGMENT_MEASUREMENT_SEARCH_EXECUTORHTTPPOST_H
