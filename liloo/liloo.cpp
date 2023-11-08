#include <map>
#include <mutex>
#include <iostream>

#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

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
            for (int i = 0; i < 4; ++i)
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
        int read_fd;
        int write_fd;

    public:
        EventFd() {
            #ifdef __linux__
                read_fd = eventfd(0, EFD_NONBLOCK);
                write_fd = read_fd;
            #else
                int p[2];
                pipe(p);
                read_fd = p[0];
                write_fd = p[1];
            #endif
        }

        void ack() {
            uint64_t tmp;
            if (read(read_fd, &tmp, sizeof(uint64_t)) == -1) {
                throw std::system_error(errno, std::generic_category(), "Failed to read from eventfd");
            }
        }

        void notify(uint64_t new_event_flag = 1) {
            if (write(write_fd, &new_event_flag, sizeof(uint64_t)) == -1) {
                throw std::system_error(errno, std::generic_category(), "Failed to write to eventfd");
            }
        }

        int fd() {
            return read_fd;
        }

        ~EventFd() {
            close(read_fd);
            close(write_fd);
        }
    };

    typedef uint64_t EventId;

    class CompletitionQueue {
        std::vector<std::pair<EventId, pybind11::object>> task_results_;
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
            std::unique_lock lock(task_results_mut_);
            std::vector<std::pair<EventId, pybind11::object>> results = std::move(task_results_);
            return pybind11::cast(results);
        }

        void AppendTaskResult(EventId event_id, pybind11::object result) {
            std::unique_lock lock(task_results_mut_);
            task_results_.emplace_back(event_id, std::move(result));
            events_queue_.notify();
        }

        int GetEventFD() {
            return events_queue_.fd();
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
            // std::cout << "End task " << event_id << '\n';
            liloo::CompletitionQueue::Instance().AppendTaskResult(event_id, pybind11::cast("TaskResult-" + std::to_string(event_id)));
        };

        processor_.EnqueueTask(std::move(fn));
        return event_id;
    }
};



pybind11::object get_completed_results() {
    return liloo::CompletitionQueue::Instance().GetCompletedResults();
}

int get_event_fd() {
    return liloo::CompletitionQueue::Instance().GetEventFD();
}


PYBIND11_MODULE(asyncie, m) {
    m.def("get_completed_results", &get_completed_results);
    m.def("get_event_fd", &get_event_fd);

    pybind11::class_<Model>(m, "Model")
        .def(pybind11::init<>())
        .def("forward", &Model::forward);
}

