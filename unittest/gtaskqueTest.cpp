//
// Created by SG Lee on 2019/11/02.
//

#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include "../src/InitializationExample.h"
#include "../src/ExecutorStream.h"
#include "../src/gtaskque.h"

using namespace std;

class gtaskqueTest: public ::testing::Test {
protected:
    gtaskqueTest() {
    }
    ~gtaskqueTest() {}
    virtual void SetUp() {
        GExecutorInterface<string,Attribute> *executorStream =
                InitializationExample::registerExecutorStream(nullptr);
        GExecutorInterface<string,Attribute> *executorThrow =
                InitializationExample::registerExecutorThrow(nullptr);
        if(executorStream) {
            InitializationExample::registerTaskQueStream(
                    typeid(ExecutorStream<string,Attribute>).name(),
                    executorStream);
        }
        if(executorThrow) {
            InitializationExample::registerTaskQueStream(
                    typeid(ExecutorThrow<string,Attribute>).name(),
                    executorThrow);
        }
        maxCount = 10;
    }
    virtual void TearDown() {
    }
private:
public:
    int maxCount = 0;
};

TEST_F(
        gtaskqueTest ,
        push_back) {
    std::chrono::steady_clock::time_point startClock = std::chrono::steady_clock::now();
    TaskQueManager &taskQueManager = TaskQueManager::get_instance();
    gtaskque<string,Attribute> *taskQue =
            taskQueManager.get_taskque(typeid(ExecutorStream<string,Attribute>).name());
    int maxFrontBufferCount = 0;
    for(size_t i = 0; i < maxCount; i++) {
        taskQue->pushBack(to_string(i));
        maxFrontBufferCount = max(maxFrontBufferCount, (int)taskQue->getFrontBufferSize());
        cout << ">>>>> " << taskQue->getFrontBufferSize() << endl;
    }
    taskQue->doAutoExecution(false);
    while(taskQue->isRunning()) {
        cout << "waiting to finish thread..." << endl;
        usleep(1000);
    }
    ASSERT_EQ(0, taskQue->getFrontBufferSize());
    ASSERT_TRUE(taskQue->areAllTasksExecuted());
    cout << "maxFrontBufferCount: " << maxFrontBufferCount << endl;
    std::chrono::steady_clock::time_point endClock = std::chrono::steady_clock::now();
}

TEST_F(
        gtaskqueTest,
        _throw) {
    TaskQueManager &taskQueManager = TaskQueManager::get_instance();
    gtaskque<string,Attribute> *taskQue =
            taskQueManager.get_taskque(typeid(ExecutorThrow<string,Attribute>).name());
    int maxFrontBufferCount = 0;
    for(size_t i = 0; i < maxCount; i++) {
        taskQue->pushBack(to_string(i));
        maxFrontBufferCount = max(maxFrontBufferCount, (int)taskQue->getFrontBufferSize());
        cout << ">>>>> " << taskQue->getFrontBufferSize() << endl;
    }
    taskQue->doAutoExecution(false);
    while(taskQue->isRunning()) {
        cout << "waiting to finish thread..." << endl;
        usleep(100000);
    }
    ASSERT_EQ(0, taskQue->getFrontBufferSize());
    ASSERT_TRUE(taskQue->areAllTasksExecuted());
    cout << "maxFrontBufferCount: " << maxFrontBufferCount << endl;

}
