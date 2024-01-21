#include "core.h"

#include <map>
#include <mutex>
#include <iostream>

#include <condition_variable>
#include <functional>

#ifdef __linux__
    #include <sys/eventfd.h>
#else
    #include <unistd.h>
#endif

namespace liloo {

    EventFd::EventFd() {
        #ifdef __linux__
            read_fd_ = eventfd(0, EFD_NONBLOCK);
            write_fd_ = read_fd_;
        #else
            int p[2];
            pipe(p);
            read_fd_ = p[0];
            write_fd_ = p[1];
        #endif
    }

    EventFd::~EventFd() {
        close(read_fd_);
        close(write_fd_);
    }

    void EventFd::Ack() {
        uint64_t tmp;
        if (read(read_fd_, &tmp, sizeof(uint64_t)) == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to read from eventfd");
        }
    }

    void EventFd::Notify(uint64_t new_event_flag) {
        if (write(write_fd_, &new_event_flag, sizeof(uint64_t)) == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to write to eventfd");
        }
    }

    int EventFd::GetFD() const {
        return read_fd_;
    }

    void CompletitionQueue::Promise::Resolve(std::function<pybind11::object()> callback) const {
        cq_.AppendTaskResult(event_id_, std::move(callback));
    }

    void CompletitionQueue::Promise::Reject(std::function<pybind11::object()> callback) const {
        cq_.AppendTaskResult(event_id_, std::move(callback));
    }

    EventId CompletitionQueue::Promise::GetEventId() const {
        return event_id_;
    }

    std::pair<typename CompletitionQueue::Promise, EventId> CompletitionQueue::MakeContract() {
        const auto event_id = GenerateEventId();
        return { Promise(*this, event_id), event_id };
    }

    /*
        We need to use std::function callback instead of pure pybind::object
        because PyObject can be created only with aquired GIL.
        In other way segmentation fault caused.

        To avoid multiple GIL aquiring we build objects in `GetCompletedResults()` that
        called only from python and already with aquired GIL.
    */
    pybind11::object CompletitionQueue::GetCompletedResults() {
        std::unique_lock lock(task_results_mut_);

        std::vector<std::pair<EventId, pybind11::object>> results;
        for (const auto& [event_id, callback] : task_results_)
            results.emplace_back(event_id, callback());

        task_results_.clear();
        return pybind11::cast(results);
    }

    /*
        Arguments
        ---------

        `callback` - function that will be called later with aquired
        GIL (Creating python object possible only with GIL). Should work as fast as possible
        and has not got system calls (That will block python event loop).

    */
    void CompletitionQueue::AppendTaskResult(EventId event_id, std::function<pybind11::object()> callback) {
        // TODO: Use move semantic for callback

        std::unique_lock lock(task_results_mut_);
        task_results_.emplace_back(event_id, std::move(callback));
        events_queue_.Notify();
    }

    int CompletitionQueue::GetEventFD() const {
        return events_queue_.GetFD();
    }

    EventId CompletitionQueue::GenerateEventId() {
        return ++event_counter_;
    }

}

//// ---------------------------------------------------------------------
// namespace utils {

//     struct Task {
//         std::function<void()> target_function{};

//         Task() { }

//         Task(std::function<void()> fn) : target_function(std::move(fn))
//         { }

//         void operator() () {
//             return target_function();
//         }
//     };

//     class TaskProcessor {
//     private:
//         std::atomic<bool> running_{true};
//         std::vector<std::thread> workers_;

//         std::queue<Task> tasks_queue_;
//         std::condition_variable queue_has_task_;
//         std::mutex queue_mut_;

//     public:

//         void BackgroundWorker() {
//             while (running_) {
//                 Task task;
//                 {
//                     std::unique_lock lock(queue_mut_);
//                     queue_has_task_.wait(lock, [this]() { return !tasks_queue_.empty() || !running_; });
//                     if (!running_)
//                         break;
//                     task = tasks_queue_.front();
//                     tasks_queue_.pop();
//                 }

//                 try {
//                     task();
//                 }
//                 catch (...) {
//                     std::cout << "[WARNING] Exception in task" << std::endl;
//                 }
//             }
//         }

//         void Initialize() {
//             for (int i = 0; i < 8; ++i)
//                 workers_.emplace_back(
//                     std::move(std::thread(&TaskProcessor::BackgroundWorker, this))
//                 );
//         }

//         TaskProcessor() {
//             Initialize();
//         }

//         void EnqueueTask(Task task) {
//             std::unique_lock lock(queue_mut_);
//             tasks_queue_.push(std::move(task));
//             queue_has_task_.notify_one();
//         }

//         void EnqueueTask(std::function<void()> target) {
//             std::unique_lock lock(queue_mut_);
//             // tasks_queue_.push(std::move(task));
//             tasks_queue_.emplace(std::move(target));
//             queue_has_task_.notify_one();
//         }

//         ~TaskProcessor() {
//             running_ = false;
//             {
//                 std::unique_lock lock(queue_mut_);
//                 queue_has_task_.notify_all();
//             }

//             for (size_t i = 0; i < workers_.size(); ++i)
//                 workers_[i].join();
//         }
//     };
// };

// class Model {
// private:
//     utils::TaskProcessor processor_;

// public:
//     liloo::Future forward(liloo::CompletitionQueue& cq) {
//         auto [promise, future] = cq.MakeContract();

//         processor_.EnqueueTask([promise]() {

//             const auto event_id = promise.GetEventId();

//             std::cout << "Start task" << event_id << '\n';
//             std::this_thread::sleep_for(std::chrono::seconds(1));
            
//             promise.Resolve([event_id](){ return pybind11::cast("TaskResult-" + std::to_string(event_id)); });
//         });
//         return future;
//     }

//     liloo::Future initialize(liloo::CompletitionQueue& cq, std::string configuration) {
//         auto [promise, future] = cq.MakeContract();

//         processor_.EnqueueTask([promise, configuration = std::move(configuration)]() {

//             const auto event_id = promise.GetEventId();

//             std::cout << "Start initialization: " << configuration << " " << event_id << '\n';
//             std::this_thread::sleep_for(std::chrono::seconds(2));
            
//             promise.Resolve([event_id](){ return pybind11::cast("initialization-result-" + std::to_string(event_id)); });
//         });
//         return future;
//     }
// };

PYBIND11_MODULE(libliloo, m) {
    pybind11::class_<liloo::CompletitionQueue>(m, "CompletitionQueue")
        .def(pybind11::init<>())
        .def("get_event_fd", &liloo::CompletitionQueue::GetEventFD)
        .def("get_completed_results", &liloo::CompletitionQueue::GetCompletedResults);

    // pybind11::class_<Model>(m, "Model")
    //     .def(pybind11::init<>())
    //     .def("forward", &Model::forward)
    //     .def("initialize", &Model::initialize);
}
