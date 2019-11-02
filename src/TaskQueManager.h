//
// Created by SG Lee on 2019/10/13.
//

#ifndef SEGMENT_MEASUREMENT_SEARCH_TASKQUEMANAGER_H
#define SEGMENT_MEASUREMENT_SEARCH_TASKQUEMANAGER_H

#include <map>
#include <string>
#include <unistd.h>
#include "gtaskque.h"

//class Attribute;

using namespace std;

class TaskQueManager {
private:
    TaskQueManager() {}
    virtual ~TaskQueManager() {
        for(map<string,gtaskque<string, Attribute>*>::iterator itr = this->_taskque_map.begin();
            itr != this->_taskque_map.end();
            ++itr) {
            if(itr->second == nullptr) {
                continue;
            }
            while(itr->second->isRunning()) {
                usleep(1000);
                if(itr->second->areAllTasksExecuted()) {
                    break;
                }
            }
            if(itr->second != nullptr) {
                delete itr->second;
            }
        }
    }
private:
public:
    inline static TaskQueManager &get_instance() {
        static TaskQueManager instance;
        return instance;
    }
private:
    map<string, gtaskque<string, Attribute> *> _taskque_map;
public:
    bool insert_taskque(const string &key, gtaskque<string, Attribute> *taskQue) {
        pair<map<string,gtaskque<string, Attribute> *>::iterator, bool> ret =
                _taskque_map.insert(pair<string,gtaskque<string, Attribute> *>(key, taskQue));
        return ret.second;
    }
    gtaskque<string, Attribute> *get_taskque(const string &key) const {
        map<string, gtaskque<string, Attribute> *>::const_iterator constIterator =
                _taskque_map.find(key);
        return (constIterator != _taskque_map.end()) ? constIterator->second : nullptr;
    }
public:
    bool request(const string &handler_key, const string &payload) {
        assert(!_taskque_map.empty());
        gtaskque<string, Attribute> *taskQue = this->get_taskque(handler_key);
        if(taskQue == nullptr) {
            return false;
        }
        taskQue->pushBack(payload);
        return true;
    }
};

#endif //SEGMENT_MEASUREMENT_SEARCH_TASKQUEMANAGER_H
