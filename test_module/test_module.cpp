#include <iostream>
#include <thread>
#include <functional>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <liloo/core.h>

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

        void EnqueueTask(std::function<void()> target) {
            std::unique_lock lock(queue_mut_);
            // tasks_queue_.push(std::move(task));
            tasks_queue_.emplace(std::move(target));
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

class Model {
private:
    utils::TaskProcessor processor_;

public:
    liloo::Future forward(liloo::CompletitionQueue& cq) {
        auto [promise, future] = cq.MakeContract();

        processor_.EnqueueTask([promise]() {

            const auto event_id = promise.GetEventId();

            std::cout << "Start task" << event_id << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            promise.Resolve([event_id](){ return pybind11::cast("TaskResult-" + std::to_string(event_id)); });
        });
        return future;
    }

    liloo::Future initialize(liloo::CompletitionQueue& cq, std::string configuration) {
        auto [promise, future] = cq.MakeContract();

        processor_.EnqueueTask([promise, configuration = std::move(configuration)]() {

            const auto event_id = promise.GetEventId();

            std::cout << "Start initialization: " << configuration << " " << event_id << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            promise.Resolve([event_id](){ return pybind11::cast("initialization-result-" + std::to_string(event_id)); });
        });
        return future;
    }
};

PYBIND11_MODULE(libtest_module, m) {
    pybind11::class_<Model>(m, "Model")
        .def(pybind11::init<>())
        .def("forward", &Model::forward)
        .def("initialize", &Model::initialize);
}

int main() {
    Model model;
    liloo::CompletitionQueue cq;
    model.forward(cq);
    std::cout << "HEllo" << std::endl;
}
