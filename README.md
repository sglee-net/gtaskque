#### Summary
The template header of "gtaskque.h" to execute registered tasks simultaneously.
![image](https://user-images.githubusercontent.com/33934527/39845060-847b9cce-542e-11e8-85ed-e8f72cc8595b.png)

#### Quick Start
1. Define "Executor" that will finally execute registered tasks.
2. Define GExecutorInterface<T,E> with the defined executor. E is the type of the executor and T is the data type that the executor handles. The second argument of GExecutorInterface constructor is about memory deletion of executor. If you want to let GExecutorInterface delete the memory of "Executor" automatically, set the second argument to "true" or vice versa. 
3. Define GTaskQue<T,E> with the defined GExecutorInterface.
4. Invoke "doAutoExecution(true)" that makes thread start.
5. Push your data to the Queue with "pushBack()".
6. Before exit, delete the allocated memory of GTaskQue. GExecutorInterface will be automatically deleted in the GTaskQue.

#### Example
``` c++
// template <typename T, typename E>
// T: type of data with which an executor handles
// E: data type of the executor
template <typename T, typename E>
class TestExecutor: public GExecutorInterface<T,E> {
public:
	TestExecutor(E *_attri,bool _b):GExecutorInterface<T,E>(_attri,_b) {}
	int execute(T &_arg) {
		ostringstream *oss = (ostringstream *)(this->getAttribute());
		(*oss) << string(_arg) << endl;
		return 0;
	}
};

void main() {
	// Executor
	ostringstream oss;
	// You can use GTaskQue::DEFAULT_SIZE_BACK_BUFFER
	size_t bufferSize = 10; 
	// The second argument of constructor is about memory deletion of executor.
	// In this case, "oss" is allocated on stack, 
	// you don't have to delete the memory of "oss" by force.
	// If you create the new "Executor" with new keyword 
	// and want to delete the memory of it automatically,
	// you can set the second argument to "true".
	GExecutorInterface<string,ostringstream> *executor = 
		new TestExecutor<string,ostringstream>(&oss,false);
	GTaskQue<string,ostringstream> *taskque = 
		new GTaskQue<string,ostringstream>(executor,buffer_size);

	taskque->doAutoExecution(true);
	for(size_t i=0; i<100; i++) {
		taskque->pushBack("string");
	}

	while(taskque->isRunning()) {
		usleep(1000);
		if(taskque->areAllTasksExecuted()) {
			break;
		}
	}
	if(taskque) delete taskque;
}
```
