/** 
 * @author SG Lee
 * @since 3/20/2010
 * @version 0.2
 * @description
 * This file includes two template classes. One is GTaskQue to register 
 * tasks, the other is GExecutorInterface that is the interface class 
 * to define a task. A task can be registered with synchronization support, 
 * and a set of tasks can be registered, too.  
 * The registered tasks are executed by a thread function continuously
 * by set the function of doAutoExecution true.
 * A registered task can be executed intermittently by calling 
 * doExecution(), but this method is not recommended because a thread
 * functions is generated whenever this function is called. 
 */

#ifndef __GTASKQUE_H__
#define __GTASKQUE_H__

#include <list>
#include <vector>
#include <atomic>
#include <memory>
#include <assert.h>
#include <boost/log/common.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/keywords/format.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logger = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

#ifdef __APPLE__
#define __linux__ __APPLE__
#endif

#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#include <process.h>
#endif

#ifdef _WIN32
typedef HANDLE			    MUTEX_TYPE;
typedef HANDLE			    THREADHANDLE_TYPE;
#elif __linux__
typedef pthread_mutex_t		MUTEX_TYPE;
typedef pthread_t		    THREADHANDLE_TYPE;
#endif

#define USLEEP_SCALE_FACTOR	1000
#define DEFAULT_SIZE_BACK_BUFFER 100

using namespace std;

/*
 * T : Task
 * E : Task Executor
 */
template <typename T, typename E>
class GExecutorInterface {
	template <typename S, typename Q>
	friend class GTaskQue;
protected:
	E *_attribute;
private:
	bool _automatic_attribute_deletion;
public:
	GExecutorInterface(
		E *attribute,
		const bool automatic_attribute_deletion) {
        this->_attribute = attribute;
        this->_automatic_attribute_deletion =
                automatic_attribute_deletion;
	}
	virtual ~GExecutorInterface() {
		if (this->_automatic_attribute_deletion) {
			delete this->_attribute;
		}
	}
	const E *getAttribute()const { return this->_attribute; }
	bool isAttributeDeletionAutomatically()const { 
		return this->_automatic_attribute_deletion;
	}
protected:
	GExecutorInterface()
		:_attribute(0),
         _automatic_attribute_deletion(true) {}
	GExecutorInterface(const GExecutorInterface &) = delete;
    GExecutorInterface &operator=(const GExecutorInterface &) = delete;
public:
	// blocking call
	virtual int execute(T &_arg) const { return 0; }
};

/*
 * T : Task
 * E : Task Executor
 */
template <typename T, typename E>
class gtaskque {
    void initBoostLogger() {
        logger::add_console_log(
                std::cout,
                keywords::auto_flush = true,
                boost::log::keywords::format = ">> %Message%");
        logger::add_file_log(
                keywords::target = "logs/", keywords::file_name = "gtaskque_%y%m%d_%3N.log",
                keywords::rotation_size = 10 * 1024 * 1024, // rolling with every 10MB
//                keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0), //12시마다 rotate
                keywords::auto_flush = true,
                keywords::scan_method = sinks::file::scan_matching,
                keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%");

        logger::core::get()->set_filter(logger::trivial::severity >= logger::trivial::info);
        logger::add_common_attributes();
    }
private:
#ifdef _WIN32
	static unsigned WINAPI thread_function_execution(void *_arg) {
#elif __linux__
	static void * thread_function_execution(void *arg) {
#endif
		gtaskque<T,E> *que = static_cast<gtaskque<T,E> *>(arg);
        try {
            ///////////////////
            // blocking method
            que->executeBatch();
            ///////////////////
        } catch(exception e){
            BOOST_LOG_TRIVIAL(error) << e.what();
            return 0;
        }
		return 0;
	}

#ifdef _WIN32
	static unsigned WINAPI thread_function_autoexecution(void *_arg) {
#elif __linux__
	static void * thread_function_autoexecution(void *arg) {
#endif
		gtaskque<T,E> *que = static_cast<gtaskque<T,E> *>(arg);
		que->_is_autoexecution_thread_running = true;
		while (true) {
            try {
                ////////////////////////////////////
                que->mutex_lock();
                if (que->isBackBufferExecuted()) {
                    que->copyToBackBuffer();
                }
                que->mutex_unlock();
                ////////////////////////////////////

                ///////////////////
                // blocking method
                que->executeBatch();
                ///////////////////
			} catch(exception e){
			    BOOST_LOG_TRIVIAL(error) << e.what();
                que->_is_autoexecution_thread_running=false;
                return 0;
			}

			// when quit command is requested or
			// execution mode is manual (not auto-execution)
			if (que->_is_quit_command_requested ||
				que->_autoexecution_command == false) {
				while(!que->areAllTasksExecuted()) {
				    BOOST_LOG_TRIVIAL(info)
				        << "quit command or autoexecution=false is requested. "
				        << "Tasks are still running, not executed yet";
                    try {
                        //////////////////////////////////
                        que->mutex_lock();
                        if (que->isBackBufferExecuted()) {
                            que->copyToBackBuffer();
                        }
                        que->mutex_unlock();
                        //////////////////////////////////

                        ///////////////////
                        // blocking method
                        que->executeBatch();
                        ///////////////////
                    } catch(exception e){
                        BOOST_LOG_TRIVIAL(error) << e.what();
                        que->_is_autoexecution_thread_running=false;
                        return 0;
                    }
#ifdef _WIN32
					Sleep(1);
#elif __linux__
					usleep(1*USLEEP_SCALE_FACTOR);
#endif
				}
				que->_is_autoexecution_thread_running=false;
				break;
			}
			
			if (que->_delay_between_batch != 0) {
#ifdef _WIN32
				Sleep(que->_delay_between_batch);
#elif __linux__
				usleep(que->_delay_between_batch *
					USLEEP_SCALE_FACTOR);
#endif
			}
		}
        que->_is_autoexecution_thread_running=false;
		return 0;
	}
private:
	// sleep between batch processes in autoexecution
	// unit : millisecond
	unsigned long		    _delay_between_batch;
	
	// sleep between jobs in the batch process
	// unit : millisecond
	unsigned long		    _delay_in_batch;
	
	// batch process will be invoked by setAutoExecution
	std::atomic<bool>	    _autoexecution_command;

	// Autoexecution for batch process
	std::atomic<bool>	    _is_autoexecution_thread_running;

	// This flag will be changed after calling quitThread()
	std::atomic<bool>	    _is_quit_command_requested;

	const GExecutorInterface<T,E> *_executor;
	size_t			        _size_back_buffer;

#ifdef _WIN32
	HANDLE			        _mutex;
	HANDLE			        _thread_handle_autoexecution;
#elif __linux__
	mutable pthread_mutex_t	_mutex;
	pthread_t		        _thread_handle_autoexecution;
	int			            _thread_param_autoexecution;
#endif
	bool			        _is_mutex_created;

	size_t			        _index_executor;
	list<pair<bool,T> >	    _front_buffer;
	vector<T *>		        _back_buffer;

private:
	gtaskque() = delete;
	gtaskque(const gtaskque<T,E> &) = delete;
	gtaskque<T,E> &operator=(const gtaskque<T,E> &) = delete;
public:
	gtaskque(
		const GExecutorInterface<T,E> *_executor,
		const size_t _size_back_buffer=DEFAULT_SIZE_BACK_BUFFER);
	virtual ~gtaskque();
public:
	inline void setDalayBetweenBatch(const unsigned long delay) {
		_delay_between_batch = delay;
	}
	inline void setDelayInBatch(const unsigned long delay) {
		_delay_in_batch = delay;
	}
	void initialize();
	void createMutex();
	void destroyMutex();
	size_t getFrontBufferSize()const;
	size_t getBackBufferSize()const;
	void quitThread();
	bool isRunning()const;
	int pushBack(const T &_v);
	int pushBack(const vector<T> &_v);
	int pushBack(const list<T> &_v);
	int doAutoExecution(const bool &_v);
	int doExecution();
	bool areAllTasksExecuted()const;
	void mutex_lock();
	void mutex_lock()const;
	void mutex_unlock();
	void mutex_unlock()const;
private:
	int executeTask();
	int executeBatch();
	size_t copyToBackBuffer();
	bool isBackBufferExecuted()const;
};

template <typename T, typename E>
void gtaskque<T,E>::initialize() {
#ifdef _WIN32
	_thread_handle_autoexecution = 0;
#elif __linux__
	_thread_handle_autoexecution = 0;
	_thread_param_autoexecution = 0;
#endif
	_index_executor = 0;
	_autoexecution_command = false;
	_is_autoexecution_thread_running = false;
	_is_quit_command_requested = false;
}

template <typename T, typename E>
void gtaskque<T,E>::createMutex() {
	if(_is_mutex_created) {
		return;
	}
#ifdef _WIN32
	_mutex = CreateMutex(NULL, FALSE, NULL);
#elif __linux__
	if (pthread_mutex_init(&_mutex, NULL) != 0) {
		BOOST_LOG_TRIVIAL(error) << "pthread_mutex_init error";
		throw runtime_error("pthread_mutex_init error");
	}
#endif
	_is_mutex_created = true;
}

template <typename T, typename E>
void gtaskque<T,E>::destroyMutex() {
	if (_is_mutex_created) {
#ifdef _WIN32
		CloseHandle(_mutex);
#elif __linux__
		pthread_mutex_destroy(&_mutex);
#endif
	}
	_is_mutex_created = false;
}

template <typename T, typename E>
gtaskque<T,E>::gtaskque(
	const GExecutorInterface<T,E> *executor,
	const size_t size_back_buffer) {
	assert(executor);
	if (!executor) {
	    BOOST_LOG_TRIVIAL(error) << "executor must not be null for constructor";
	    throw invalid_argument("executor must not be null for constructor");
	}

	initBoostLogger();

	initialize();

	_delay_between_batch = 1;	// milliseconds
	_delay_in_batch = 0; 	  	// milliseconds
	_is_mutex_created = false;

	this->_executor = executor;
	this->_size_back_buffer = size_back_buffer;
	this->_back_buffer.assign(size_back_buffer,nullptr);

	// create Mutex
	createMutex();
}

template <typename T, typename E>
gtaskque<T,E>::~gtaskque() {
	quitThread();

	assert(getFrontBufferSize()==0);
//	if(getFrontBufferSize()!=0) {
//		throw bad_exception("check your code, quitThread()");
//	}
	// check the code, maybe some problems in quitThread()

	// if GExecutorInterface is not const, the below code is available
//	if (executor) {
//		delete executor;
//		executor = nullptr;
//	}
	
	destroyMutex();
}

template <typename T, typename E>
size_t gtaskque<T,E>::getFrontBufferSize() const {
	mutex_lock();
	size_t size = _front_buffer.size();
	mutex_unlock();
	return size;
}

template <typename T, typename E>
size_t gtaskque<T,E>::getBackBufferSize() const {
	return _back_buffer.size();
}

template <typename T, typename E>
void gtaskque<T,E>::quitThread() {
	if (_is_quit_command_requested) {
		BOOST_LOG_TRIVIAL(info) << "quitThread() is requested again, no response.";
		return;
	}

	_is_quit_command_requested = true;

	// quit and wait until autoexecution will be shutdown
	doAutoExecution(false);

	while (_is_autoexecution_thread_running) {
#ifdef _WIN32
		Sleep(1);
#elif __linux__
		usleep(1*USLEEP_SCALE_FACTOR);
#endif
		BOOST_LOG_TRIVIAL(info) << "waiting for completion of executions";
	}

	_is_quit_command_requested = false;
}

template <typename T, typename E>
bool gtaskque<T,E>::isRunning() const {
	if(_is_autoexecution_thread_running || _is_quit_command_requested ||
       !areAllTasksExecuted()) {
		return true;
	}
	else {
		return false;
	}
}

// return value
// 0  : normal
// -1 : The quit request is already called
template <typename T, typename E>
int gtaskque<T,E>::pushBack(const T &v) {
	if (_is_quit_command_requested) {
		BOOST_LOG_TRIVIAL(warning)
		    << "quitThread() is called, so pushBack() does not work";
		return -1;
	}

	/////////////
	// lock
	mutex_lock();
	/////////////
	_front_buffer.push_back(pair<bool,T>(false,v));
	///////////////
	// unlock
	mutex_unlock();
	///////////////
	
	return 0;
}

// return value
// 0  : normal
// -1 : The quit request is already called
template <typename T, typename E>
int gtaskque<T,E>::pushBack(const vector<T> &v) {
	if (_is_quit_command_requested) {
        BOOST_LOG_TRIVIAL(warning)
            << "quitThread() is called, so pushBack() does not work";
		return -1;
	}

	//////////////
	// lock
	mutex_lock();
	//////////////
	typename vector<T>::const_iterator citr = v.begin();
	while (citr != v.end()) {
		_front_buffer.push_back(pair<bool, T>(false, (*citr)));
		citr++;
	}
	///////////////
	// unlock
	mutex_unlock();
	///////////////
	
	return 0;
}

// return value
// 0  : normal
// -1 : The quit request is already called
template <typename T, typename E>
int gtaskque<T,E>::pushBack(const list<T> &v) {
	if (_is_quit_command_requested) {
        BOOST_LOG_TRIVIAL(warning)
            << "quitThread() is called, so pushBack() does not work";
		return -1;
	}

	//////////////
	// lock
	mutex_lock();
	//////////////
	typename list<T>::const_iterator citr = v.begin();
	while (citr != v.end()) {
		_front_buffer.push_back(pair<bool, T>(false, (*citr)));
		citr++;
	}
	///////////////
	// unlock
	mutex_unlock();
	///////////////
	
	return 0;
}

// return
// 0 : OK (true, false)
// 1 : Execution is already running, so this call does not effect on it
// 2 : quitThread() is already requested, so this call does not effect on it
//
template <typename T, typename E>
int gtaskque<T,E>::doAutoExecution(const bool &v) {
	if(_is_quit_command_requested && v) {
        BOOST_LOG_TRIVIAL(warning)
            << "quitThread() is called, so pushBack() does not work";
		return 2;
	}

	assert(this->_executor);
	if (!this->_executor) {
        BOOST_LOG_TRIVIAL(error) << "no task to execute";
        throw logic_error("no task to execute");
	}

	// autoexecution is already running
	if (_autoexecution_command && v) {
		BOOST_LOG_TRIVIAL(warning)
		    << "autoexecution is already running";
		return 1;
	}

	// atomic bool
	_autoexecution_command = v;

	// false => autoexecution thread will be shut down
	if (_autoexecution_command == false) {
		return 0;
	}
	
#ifdef _WIN32
	_thread_handle_autoexecution =
		(HANDLE)_beginthreadex(NULL, 0, thread_function_autoexecution, this, 0, NULL);
#elif __linux__
	if (pthread_create(&_thread_handle_autoexecution,
		NULL, thread_function_autoexecution, this) != 0) {
	    BOOST_LOG_TRIVIAL(error) << "pthread_create error";
		throw runtime_error("pthread_create error");
	}
#endif

	return 0;
}

// return
// 0 : OK 
// throw : autoexecution is running or other tasks are running
template <typename T, typename E>
int gtaskque<T,E>::doExecution() {
	assert(this->_executor);
	if (!this->_executor) {
	    BOOST_LOG_TRIVIAL(error) << "no task to execute";
		throw logic_error("no task to execute");
	}

	assert(!this->_is_autoexecution_thread_running);
	if (this->_is_autoexecution_thread_running) {
        BOOST_LOG_TRIVIAL(warning)
            << "autoexecution is already running, no response";
        // throw logic_error();
		return -1;
	}

	if (!areAllTasksExecuted()) {
        BOOST_LOG_TRIVIAL(warning)
            << "Previous execution is running, please wait until finish";
        // throw logic_error();
		return -1;
	}

#ifdef _WIN32
	_thread_handle_autoexecution =
		(HANDLE)_beginthreadex(NULL, 0, thread_function_execution, this, 0, NULL);
#elif __linux__
	if (pthread_create(&_thread_handle_autoexecution,
		NULL, thread_function_execution, this) != 0) {
	    BOOST_LOG_TRIVIAL(error) << "pthread_create error";
		throw runtime_error("pthread_create error");
	}
#endif

	return 0;
}

template <typename T, typename E>
bool gtaskque<T,E>::areAllTasksExecuted() const {
	return (isBackBufferExecuted() && getFrontBufferSize() == 0)? true : false;
}

template <typename T, typename E>
void gtaskque<T,E>::mutex_lock() {
	if (_is_mutex_created) {
#ifdef _WIN32
		WaitForSingleObject(_mutex, INFINITE);
#elif __linux__
		pthread_mutex_lock(&_mutex);
#endif
	}
}

template <typename T, typename E>
void gtaskque<T,E>::mutex_lock() const {
	if (_is_mutex_created) {
#ifdef _WIN32
		WaitForSingleObject(_mutex, INFINITE);
#elif __linux__
		pthread_mutex_lock(&_mutex);
#endif
	}
}

template <typename T, typename E>
void gtaskque<T,E>::mutex_unlock() {
	if (_is_mutex_created) {
#ifdef _WIN32
		ReleaseMutex(_mutex);
#elif __linux__
		pthread_mutex_unlock(&_mutex);
#endif
	}
}

template <typename T, typename E>
void gtaskque<T,E>::mutex_unlock() const {
	if (_is_mutex_created) {
#ifdef _WIN32
		ReleaseMutex(_mutex);
#elif __linux__
		pthread_mutex_unlock(&_mutex);
#endif
	}
}

// private /////////////////////////////////////////////////////////////////

template <typename T, typename E>
int gtaskque<T,E>::executeTask() {
	if (areAllTasksExecuted()) {
	    BOOST_LOG_TRIVIAL(warning) << "no task to execute";
		return -1;
	}

	try {
        // blocking call
        this->_executor->execute(*(_back_buffer[_index_executor]));
	} catch (exception e) {
	    BOOST_LOG_TRIVIAL(error) << "_executor exception, " << e.what();
	}
	_back_buffer[_index_executor] = nullptr;
	_index_executor++;
	
	if (_index_executor >= getBackBufferSize()) {
		_index_executor = 0;
	}
	
	return 0;
}

template <typename T, typename E>
int gtaskque<T,E>::executeBatch() {
    assert(_index_executor < getBackBufferSize());
	if (_index_executor >= getBackBufferSize()) {
	    BOOST_LOG_TRIVIAL(error) << "Execution index is bigger than buffer size";
		throw logic_error("Execution index is bigger than buffer size");
	}

	for (size_t i = _index_executor;
		i < this->getBackBufferSize();
		i++) {
		if (_back_buffer[i] == nullptr) {
			break;
		}

		try {
            // blocking call
            this->_executor->execute(*(_back_buffer[i]));
		} catch (exception e) {
            BOOST_LOG_TRIVIAL(error) << "_executor exception, " << e.what();
		}
		_back_buffer[i] = nullptr;
		_index_executor++;
		
		if (_delay_in_batch != 0) {
#ifdef _WIN32
			Sleep(delay_in_batch);
#elif __linux__
			usleep(_delay_in_batch*USLEEP_SCALE_FACTOR);
#endif
		}
	}
	
	_index_executor = 0;

	return 0;
}

template <typename T, typename E>
size_t gtaskque<T,E>::copyToBackBuffer() {
	assert(isBackBufferExecuted());
	if (!isBackBufferExecuted()) {
	    BOOST_LOG_TRIVIAL(error) << "BackBuffer is not executed yet";
		throw logic_error("BackBuffer is not executed yet");
	}
	size_t copied_count = 0;
	std::fill(_back_buffer.begin(), _back_buffer.end(), nullptr);
	_index_executor = 0;
	typename list<pair<bool,T> >::iterator itr = _front_buffer.begin();
	while (itr != _front_buffer.end()) {
		if (itr->first == true) {
			_front_buffer.pop_front();
			itr = _front_buffer.begin();
			continue;
		} else {
			if (copied_count >= getBackBufferSize()) {
				break;
			}
			itr->first = true;
			_back_buffer[copied_count] = &(itr->second);
            copied_count++;
		}
		++itr;
	}
	return copied_count;
}

template <typename T, typename E>
bool gtaskque<T,E>::isBackBufferExecuted() const {
	for (size_t i = 0; i < getBackBufferSize(); i++) {
		if (_back_buffer[i] != nullptr) {
			return false;
		}
	}
	return true;
}

#endif
