#pragma once

#include <mutex>
#include <atomic>

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#ifdef __linux__
    #include <sys/eventfd.h>
#else
    #include <unistd.h>
#endif


namespace liloo __attribute__((visibility("default"))) {

    class EventFd {
    private:
        int read_fd_;
        int write_fd_;

    public:
        EventFd();
        ~EventFd();

        void Ack();
        void Notify(uint64_t new_event_flag = 1);
        int GetFD() const;
    };

    typedef uint64_t EventId;

    class CompletitionQueue {
        std::vector< std::pair<EventId, std::function<pybind11::object()>> > task_results_;
        std::mutex task_results_mut_;

        EventFd events_queue_;
        std::atomic<EventId> event_counter_;

    public:

        class Promise {
        private:
            EventId event_id_;
            CompletitionQueue& cq_;

        public:
            Promise(CompletitionQueue& cq, EventId event_id) : event_id_(event_id), cq_(cq) { }

            /*
                Arguments
                ---------

                `callback` - function that will be called later with aquired
                GIL (Creating python object possible only with GIL). Should work as fast as possible
                and has not got system calls (That will block python event loop).
            */
            __attribute__((visibility("default"))) void Resolve(std::function<pybind11::object()> callback) const;
            __attribute__((visibility("default"))) void Reject(std::function<pybind11::object()> callback) const;
            EventId GetEventId() const;
        };

        CompletitionQueue() = default;

        std::pair<Promise, EventId> MakeContract(); 
        pybind11::object GetCompletedResults();
        int GetEventFD() const;

        EventId GenerateEventId();
        void AppendTaskResult(EventId event_id, std::function<pybind11::object()> callback);
    };

    typedef EventId Future;
}
