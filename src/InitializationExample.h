//
// Created by SG Lee on 2019/11/01.
//

#ifndef SEGMENT_MEASUREMENT_SEARCH_INITIALIZATION_H
#define SEGMENT_MEASUREMENT_SEARCH_INITIALIZATION_H

#include "AttributeManager.h"
#include "ExecutorManager.h"
#include "TaskQueManager.h"
#include "ExecutorStream.h"
#include "ExecutorThrow.h"

class InitializationExample {
public:
    inline static int BACKBUFFERSIZE = 100;
private:
public:
//    inline static void initLogger() {
//        BoostLogger &logger = BoostLogger::get_instance();
//        logger.set_filter(logging::trivial::trace);
//    }

    inline static GExecutorInterface<string, Attribute> *registerExecutorThrow(Attribute *attribute) {
        ExecutorManager &executorManager = ExecutorManager::get_instance();
        {
            GExecutorInterface<string, Attribute> *executor =
                    new ExecutorThrow<string, Attribute>(attribute, false);
            if(!executorManager.insert_executor(typeid(ExecutorThrow<string, Attribute>).name(), executor)) {
                cout << "executor insertion failed" << endl;
                return nullptr;
            }
            return executor;
        }
    }

    inline static GExecutorInterface<string, Attribute> *registerExecutorStream(Attribute *attribute) {
        ExecutorManager &executorManager = ExecutorManager::get_instance();
        {
            GExecutorInterface<string, Attribute> *executor =
                    new ExecutorStream<string, Attribute>(attribute, false);
            if(!executorManager.insert_executor(typeid(ExecutorStream<string, Attribute>).name(), executor)) {
                cout << "executor insertion failed" << endl;
                return nullptr;
            }
            return executor;
        }
    }

    inline static void registerTaskQueStream(const string &key, const GExecutorInterface<string, Attribute> *executor) {
        TaskQueManager &taskQueManager = TaskQueManager::get_instance();
        ExecutorManager &executorManager = ExecutorManager::get_instance();
//        {
//            const GExecutorInterface<string, Attribute> *executor =
//                    executorManager.get_executor(typeid(ExecutorStream<string, Attribute>).name());
//            assert(executor != nullptr);
//            if(executor == nullptr) {
//                throw invalid_argument("Executor should be defined before TaskQue creation");
//            }
//            // register TaskQue
//            gtaskque<string, Attribute> *taskQue =
//                    new gtaskque<string, Attribute>(executor, BACKBUFFERSIZE);
//            if(!taskQueManager.insert_taskque(typeid(ExecutorStream<string, Attribute>).name(), taskQue)) {
//                cout << "insertion of TaskQue for measurement search failed" << endl;
//            }
//            taskQue->doAutoExecution(true);
//        }
        gtaskque<string, Attribute> *taskQue =
                new gtaskque<string, Attribute>(executor, BACKBUFFERSIZE);
        if(!taskQueManager.insert_taskque(key, taskQue)) {
            cout << "insertion of TaskQue failed" << endl;
            return;
        }
        taskQue->doAutoExecution(true);
    }
};
#endif //SEGMENT_MEASUREMENT_SEARCH_INITIALIZATION_H
