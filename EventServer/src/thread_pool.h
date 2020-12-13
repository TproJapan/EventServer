#pragma once
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
class thread_pool
{
    boost::asio::io_service& io_service_;
    boost::shared_ptr<boost::asio::io_service::work> work_;
    boost::thread_group group_;
    boost::thread** threads_;//boost::threadポインタの配列
public:
    thread_pool(boost::asio::io_service& io_service, std::size_t size)
        : io_service_(io_service)
    {
        threads_ = new boost::thread * [(int)size];//動的確保
        work_.reset(new boost::asio::io_service::work(io_service_));

        for (std::size_t i = 0; i < size; ++i) {
            threads_[i] = group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
        }
    }

    ~thread_pool()
    {
        work_.reset();
        group_.join_all();
        delete[] threads_;//動的開放
    }

    template <class F>
    void post(F f)
    {
        io_service_.post(f);
    }

    void terminateAllThreads() {
        //group_.interrupt_all();

        for (std::size_t i = 0; i < group_.size(); ++i) {
            threads_[i]->interrupt();
        }

    }
};
