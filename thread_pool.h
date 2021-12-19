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
public:
//コンストラクタの中より手前でメンバー変数を初期化したい場合は「:メンバー変数」の書き方をする
    thread_pool(boost::asio::io_service& io_service, std::size_t size)
        : io_service_(io_service)
    {
        work_.reset(new boost::asio::io_service::work(io_service_));
        //group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));  //konishi
    	for (std::size_t i = 0; i < size; ++i) {                                           // konishi
            group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));// konishi
        }                                                                                  // konishi
    }

    ~thread_pool()
    {
        work_.reset();
        group_.join_all();
    }

    template <class F>
    void post(F f)
    {
        io_service_.post(f);
    }

    void terminateAllThreads() {
        group_.interrupt_all();
    }
};
