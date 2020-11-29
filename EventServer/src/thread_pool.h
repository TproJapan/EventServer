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
        for (std::size_t i = 0; i < group_.size(); ++i) {
            //boost::thread_group管理下から解放。joinで待たなくなる
            group_.remove_thread(threads_[i]);

            //c++11 std::threadを明示的にterminateする
            TerminateThread(threads_[i]->native_handle(), (DWORD)0x00);

            //terminateできたか確認
            DWORD result;
            GetExitCodeThread(threads_[i]->native_handle(), &result);
            printf("result:%ui\n", result);//どうやら生きているっぽい

            //threadハンドルクローズ
            CloseHandle(threads_[i]->native_handle());
        }
    }
};
