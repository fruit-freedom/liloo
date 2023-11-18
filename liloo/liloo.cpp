#include <map>
#include <mutex>
#include <iostream>

#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <functional>


#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#ifdef __linux__
    #include <sys/eventfd.h>
#else
    #include <unistd.h>
#endif

namespace utils {

    struct Task {
        std::function<void()> target_function{};

        Task() { }

        Task(std::function<void()> fn) : target_function(std::move(fn))
        { }

        void operator() () {
            return target_function();
        }
    };

    class TaskProcessor {
    private:
        std::atomic<bool> running_{true};
        std::vector<std::thread> workers_;

        std::queue<Task> tasks_queue_;
        std::condition_variable queue_has_task_;
        std::mutex queue_mut_;

    public:

        void BackgroundWorker() {
            while (running_) {
                Task task;
                {
                    std::unique_lock lock(queue_mut_);
                    queue_has_task_.wait(lock, [this]() { return !tasks_queue_.empty() || !running_; });
                    if (!running_)
                        break;
                    task = tasks_queue_.front();
                    tasks_queue_.pop();
                }

                try {
                    task();
                }
                catch (...) {
                    std::cout << "[WARNING] Exception in task" << std::endl;
                }
            }
        }

        void Initialize() {
            for (int i = 0; i < 8; ++i)
                workers_.emplace_back(
                    std::move(std::thread(&TaskProcessor::BackgroundWorker, this))
                );
        }

        TaskProcessor() {
            Initialize();
        }

        void EnqueueTask(Task task) {
            std::unique_lock lock(queue_mut_);
            tasks_queue_.push(std::move(task));
            queue_has_task_.notify_one();
        }

        ~TaskProcessor() {
            running_ = false;
            {
                std::unique_lock lock(queue_mut_);
                queue_has_task_.notify_all();
            }

            for (size_t i = 0; i < workers_.size(); ++i)
                workers_[i].join();
        }
    };
};

namespace liloo {

    class EventFd {
    private:
        int read_fd_;
        int write_fd_;

    public:
        EventFd() {
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

        void Ack() {
            uint64_t tmp;
            if (read(read_fd_, &tmp, sizeof(uint64_t)) == -1) {
                throw std::system_error(errno, std::generic_category(), "Failed to read from eventfd");
            }
        }

        void Notify(uint64_t new_event_flag = 1) {
            if (write(write_fd_, &new_event_flag, sizeof(uint64_t)) == -1) {
                throw std::system_error(errno, std::generic_category(), "Failed to write to eventfd");
            }
        }

        int GetFD() {
            return read_fd_;
        }

        ~EventFd() {
            close(read_fd_);
            close(write_fd_);
        }
    };

    typedef uint64_t EventId;

    class CompletitionQueue {
        std::vector<
            std::pair<EventId,std::function<pybind11::object()>>
        > task_results_;

        std::mutex task_results_mut_;

        EventFd events_queue_;
        std::atomic<EventId> event_counter_;
    public:
        CompletitionQueue() = default;

        static CompletitionQueue& Instance() {
            static CompletitionQueue cq;
            return cq;
        }

        pybind11::object GetCompletedResults() {
            /*
            We need to use std::function callback instead of pure pybind::object
            because PyObject can be created only with aquired GIL.
            In other way segmentation fault caused.

            To avoid multiple GIL aquiring we build objects in `GetCompletedResults()` that
            called only from python and already with aquired GIL.
            */
            std::unique_lock lock(task_results_mut_);

            std::vector<std::pair<EventId, pybind11::object>> results;
            for (const auto& [event_id, callback] : task_results_)
                results.emplace_back(event_id, callback());

            task_results_.clear();
            return pybind11::cast(results);
        }

        void AppendTaskResult(EventId event_id, std::function<pybind11::object()> callback) {
            std::unique_lock lock(task_results_mut_);
            task_results_.emplace_back(event_id, std::move(callback));
            events_queue_.Notify();
        }

        int GetEventFD() {
            return events_queue_.GetFD();
        }

        EventId GenerateEventId() {
            return ++event_counter_;
        }
    };
}

class Model {
private:
    utils::TaskProcessor processor_;

public:
    liloo::EventId forward() {
        const auto event_id = liloo::CompletitionQueue::Instance().GenerateEventId();

        std::function<void()> fn = [event_id]() {
            std::cout << "Start task" << event_id << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            liloo::CompletitionQueue::Instance().AppendTaskResult(event_id, [event_id](){
                return pybind11::cast("TaskResult-" + std::to_string(event_id));
            });
        };

        processor_.EnqueueTask(std::move(fn));
        return event_id;
    }

    liloo::EventId initialize() {
        const auto event_id = liloo::CompletitionQueue::Instance().GenerateEventId();

        std::function<void()> fn = [event_id]() {
            std::cout << "Start initialization" << event_id << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            liloo::CompletitionQueue::Instance().AppendTaskResult(event_id, [event_id](){
                return pybind11::cast("initialization-result-" + std::to_string(event_id));
            });
        };

        processor_.EnqueueTask(std::move(fn));
        return event_id;
    }
};

PYBIND11_MODULE(asyncie, m) {
    m.def("_get_completed_results", []() {
        return liloo::CompletitionQueue::Instance().GetCompletedResults();
    });

    m.def("_get_event_fd", []() {
        return liloo::CompletitionQueue::Instance().GetEventFD();
    });

    pybind11::class_<Model>(m, "Model")
        .def(pybind11::init<>())
        .def("forward", &Model::forward)
        .def("initialize", &Model::initialize);

    m.def("ntest", [](const pybind11::array_t<int>& arr) {
        std::cout << "itemsize: " << arr.itemsize() << std::endl;
        std::cout << "ndim: " << arr.ndim() << std::endl;

        std::cout << "shape: [";
        for (int i = 0; i < arr.ndim(); ++i)
            std::cout << arr.shape()[i] << (i + 1 == arr.ndim() ? "" : ", ");
        std::cout << "]" << std::endl;

        std::cout << "strides: [";
        for (int i = 0; i < arr.ndim(); ++i)
            std::cout << arr.strides()[i] << (i + 1 == arr.ndim() ? "" : ", ");
        std::cout << "]" << std::endl;

        int size = 1;
        for (int dim = 0; dim < arr.ndim(); ++dim)
            size *= arr.shape(dim);

        auto data = arr.data();
        for (int i = 0; i < size; ++i)
            std::cout << data[i] << ' ';
    });
}

