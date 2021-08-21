#include "BoostLog.h"

///////////////////////////////////////////////////////////////////////////////
// write_log
///////////////////////////////////////////////////////////////////////////////
void write_log(int level, char* message)
{
    using namespace logging::trivial;
    src::severity_logger< severity_level > lg;

    switch(level)
    {
    case 0:
        BOOST_LOG_SEV(lg, trace) << message;
        break;
    case 1:
        BOOST_LOG_SEV(lg, debug) << message;
        break;
    case 2:
        BOOST_LOG_SEV(lg, info) << message;
        break;
    case 3:
        BOOST_LOG_SEV(lg, warning) << message;
        break;
    case 4:
        BOOST_LOG_SEV(lg, error) << message;
        break;
    case 5:
        BOOST_LOG_SEV(lg, fatal) << message;
        break;
    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// init
///////////////////////////////////////////////////////////////////////////////
void init(int level, const char* log_dir, const char* log_filename)
{
    namespace expr = boost::log::expressions;

    char file_name[1024];
    sprintf_s(file_name, "%s\\%s", log_dir, log_filename);

    logging::add_file_log
    (
        keywords::file_name = file_name,
        keywords::rotation_size = 1 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        //keywords::time_based_rotation = sinks::file::rotation_at_time_interval(boost::posix_time::seconds(30)),
        //keywords::target = "c:\\tmp",
        //keywords::max_size = 5 * 1024,
        //    keywords::format = "[%TimeStamp%]: %Message%"
        keywords::format =
        (
            expr::stream
            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y/%m/%d %H:%M:%S")
            << ": <" << logging::trivial::severity
            << "> " << expr::smessage
            )
    );

    switch (level)
    {
    case 0:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::trace);
        break;
    case 1:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::debug);
        break;
    case 2:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
        break;
    case 3:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::warning);
        break;
    case 4:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::error);
        break;
    case 5:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::fatal);
        break;
    default:
        break;
    }
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
        Sleep(5000);
    }

    return;
}