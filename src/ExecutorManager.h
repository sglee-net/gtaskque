//
// Created by SG Lee on 2019/10/12.
//

#ifndef SEGMENT_MEASUREMENT_SEARCH_EXECUTORMANAGER_H
#define SEGMENT_MEASUREMENT_SEARCH_EXECUTORMANAGER_H

#include <map>
#include <string>
#include "gtaskque.h"

class Attribute;

using namespace std;

class ExecutorManager {
private:
    ExecutorManager() {}
    ~ExecutorManager() {
        cout << "size of executor_map " << this->executor_map.size() << endl;
        // GExecutorInterface will not be destroyed inside gtaskque::gtaskque() because GExecutorInterface is const
        for(map<string, const GExecutorInterface<string, Attribute> *>::iterator itr=this->executor_map.begin();
            itr!=this->executor_map.end();
            ++itr) {
            if(itr->second != nullptr && !itr->second->isAttributeDeletionAutomatically()) {
                cout << "executor " << itr->second << " is deleted " << endl;
                delete itr->second;
            }
        }
        this->executor_map.clear();
    }
    ExecutorManager(const ExecutorManager &) = delete;
    ExecutorManager &operator=(const ExecutorManager &) = delete;
public:
    inline static ExecutorManager &get_instance() {
        static ExecutorManager instance;
        return instance;
    }
private:
    map<string, const GExecutorInterface<string, Attribute> *> executor_map;
public:
    bool insert_executor(const string &key, const GExecutorInterface<string, Attribute> *executor) {
        pair<map<string, const GExecutorInterface<string, Attribute> *>::iterator, bool> rt =
                this->executor_map.insert(
                        pair<string, const GExecutorInterface<string, Attribute> *>(
                                key,
                                executor
                        ));
        return rt.second;
    }
    const GExecutorInterface<string, Attribute> *get_executor(const string &key)const {
        map<string, const GExecutorInterface<string, Attribute> *>::const_iterator citr =
                this->executor_map.find(key);
        return (citr!=this->executor_map.end())? citr->second : nullptr;
    }
};

#endif //SEGMENT_MEASUREMENT_SEARCH_EXECUTORMANAGER_H
