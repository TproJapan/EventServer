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
    boost::thread** threads_;//boost::thread�|�C���^�̔z��
public:
    thread_pool(boost::asio::io_service& io_service, std::size_t size)
        : io_service_(io_service)
    {
        threads_ = new boost::thread * [(int)size];//���I�m��
        work_.reset(new boost::asio::io_service::work(io_service_));

        for (std::size_t i = 0; i < size; ++i) {

            //group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
            threads_[i] = group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
            printf("handle=%d\n", threads_[i]->native_handle());
            //��thread�N���X�̃|�C���^�z��ł͂Ȃ��Anative_handl������
            //  �ێ�����z��ł��ǂ������B
        }
    }

    ~thread_pool()
    {
        work_.reset();
        group_.join_all();
        delete[] threads_;//���I�J��
    }

    template <class F>
    void post(F f)
    {
        io_service_.post(f);
    }

    void terminateAllThreads() {
        for (std::size_t i = 0; i < group_.size(); ++i) {
            //boost::thread_group�Ǘ����������Bjoin�ő҂��Ȃ��Ȃ�
            //group_.remove_thread(threads_[i]);
            // remove_thread���s����join����Ȃ��̂�abort����������B
            // thread_pool�̃f�X�g���N�^��join�����邽�߂�remove�͎��߂�B

            //c++11 std::thread�𖾎��I��terminate����
            // TerminateThread(threads_[i]->native_handle(), (DWORD)0x00);

            // �I���R�[�h��0x00���Ƌ�ʂ��ɂ����̂�0x04�ŏI��点��
            printf("handle=%d\n", threads_[i]->native_handle());
            TerminateThread(threads_[i]->native_handle(), (DWORD)0x04);

            //terminate�ł������m�F
            DWORD result;
            //GetExitCodeThread(threads_[i]->native_handle(), &result);
            //printf("result:%ui\n", result);//�ǂ���琶���Ă�����ۂ�

            BOOL BRet = GetExitCodeThread(threads_[i]->native_handle(), &result);
            printf("BRet=%d, result:%ld\n", BRet, result);//259:�ǂ���琶���Ă�����ۂ�

            // �T�u�X���b�h���I������܂Ŗ����ҋ@
            // �{���͑S���̃X���b�h�n���h����Ώۂ�WaitForMultipleObject���g���ׂ������B
            ::WaitForSingleObject(threads_[i]->native_handle(), INFINITE);

            // �X���b�h�̏I����҂��Ă���I���R�[�h���擾���Ă���̂�0x04�ɂȂ�
            BRet = GetExitCodeThread(threads_[i]->native_handle(), &result);
            printf("BRet=%d, result:%ld\n", BRet, result);

            //thread�n���h���N���[�Y
            //CloseHandle(threads_[i]->native_handle());
            // thread�N���X�̃f�X�g���N�^��close����͂��Ȃ̂�CloseHandle�͎��߂�
        }
    }
};