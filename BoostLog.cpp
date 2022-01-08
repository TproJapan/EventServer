//#define BOOST_LOG_DYN_LINK 1	// konishi  makefileのCCFLAGに引っ越し
#include "BoostLog.h"

///////////////////////////////////////////////////////////////////////////////
// write_log
///////////////////////////////////////////////////////////////////////////////
bool write_log(int level, const char* message, ...)
{
    va_list ap;
    char* allocBuf;
    va_start(ap, message);
    int nSize = vasprintf(&allocBuf, message, ap);
    va_end(ap);

    if (nSize == 0) {
        return false;
    }

    using namespace logging::trivial;
    src::severity_logger< severity_level > lg;

    //BOOST_LOG_SEV(lg, (boost::log::v2s_mt_nt6::trivial::severity_level)level) << allocBuf;
    //BOOST_LOG_SEV(lg, level) << allocBuf;	// konishi
    //BOOST_LOG_SEV(lg, (boost::log::v2s_mt_posix::trivial::severity_level)level) << allocBuf;// konishi
	BOOST_LOG_SEV(lg, (boost::log::trivial::severity_level)level) << allocBuf;// konishi

    free(allocBuf);
    return true;
}
#if 0 // konishi
int vasprintf(char** strp, const char* fmt, va_list ap) {
    char* buf = (char*)malloc(_vscprintf(fmt, ap) + 1);
    if (buf == NULL) {
        if (strp != NULL) {
            *strp = NULL;
        }
        return -1;
    }
    *strp = buf;
    return vsprintf(buf, fmt, ap);
}
#endif //konishi
///////////////////////////////////////////////////////////////////////////////
// init
///////////////////////////////////////////////////////////////////////////////
void init(int level, const char* log_dir, const char* log_filename)
{
    namespace expr = boost::log::expressions;

    char file_name[1024];
#ifdef _WIN32 // konishi
    sprintf_s(file_name, "%s\\%s", log_dir, log_filename);
#else  // konishi
    sprintf(file_name, "%s/%s", log_dir, log_filename); // konishi
#endif // konishi

    logging::add_file_log
    (
        keywords::open_mode = (std::ios::out | std::ios::app),
        keywords::file_name = file_name,
        keywords::rotation_size = 100 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        //keywords::time_based_rotation = sinks::file::rotation_at_time_interval(boost::posix_time::seconds(30)),
        keywords::target = log_dir,
        //keywords::max_size = 5 * 1024,
        //    keywords::format = "[%TimeStamp%]: %Message%"
        keywords::auto_flush = true,
        keywords::format =
        (
            expr::stream
            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y/%m/%d %H:%M:%S")
            << ": <" << logging::trivial::severity
            << "> " << expr::smessage
            )
    );

    //logging::core::get()->set_filter(logging::trivial::severity >= (boost::log::v2s_mt_nt6::trivial::severity_level)level);//konishi
	//logging::core::get()->set_filter(logging::trivial::severity >= level); // konishi
    //logging::core::get()->set_filter(logging::trivial::severity >= (boost::log::v2s_mt_posix::trivial::severity_level)level); // konishi
    logging::core::get()->set_filter(logging::trivial::severity >= (boost::log::trivial::severity_level)level); // konishi

}

///////////////////////////////////////////////////////////////////////////////
// test1
///////////////////////////////////////////////////////////////////////////////
void test1()
{
    // フィルタなしでロギング
    // 各区分ごとにログが出力される
    BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
    BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
    BOOST_LOG_TRIVIAL(info) << "An informational severity message";
    BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
    BOOST_LOG_TRIVIAL(error) << "An error severity message";
    BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";

    const char* pName = "山田太郎";
    int score = 85;
    BOOST_LOG_TRIVIAL(fatal) << "★★　名前: " << pName << ". score:" << score;

    // ログにフィルタをかける。
    // info以上を出力し、それ以外は捨てる
    using namespace logging;
    core::get()->set_filter
    (
        trivial::severity >= trivial::info
    );

    // フィルタ付きでロギング
    // traceとdebugは出力されない
    BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
    BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
    BOOST_LOG_TRIVIAL(info) << "An informational severity message";
    BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
    BOOST_LOG_TRIVIAL(error) << "An error severity message";
    BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";

    return;
}

///////////////////////////////////////////////////////////////////////////////
// test2
///////////////////////////////////////////////////////////////////////////////
void test2()
{
    init(0, LOG_DIR_SERV, LOG_FILENAME_SERV);
    logging::add_common_attributes();

    using namespace logging::trivial;
    src::severity_logger< severity_level > lg;

    while (true) {
        BOOST_LOG_SEV(lg, trace) << "A trace severity message";
        BOOST_LOG_SEV(lg, debug) << "A debug severity message";
        BOOST_LOG_SEV(lg, info) << "An informational severity message";
        BOOST_LOG_SEV(lg, warning) << "A warning severity message";
        BOOST_LOG_SEV(lg, error) << "An error severity message";
        BOOST_LOG_SEV(lg, fatal) << "A fatal severity message";
#ifdef _WIN32 // konishi
        Sleep(5000);
#else // konishi
    	sleep(5); // konishi
#endif // konishi
    }

    return;
}