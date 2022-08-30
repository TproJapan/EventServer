#include "BoostLog.h"

///////////////////////////////////////////////////////////////////////////////
// writeLog
///////////////////////////////////////////////////////////////////////////////
bool writeLog(int level, const char* message, ...)
{
    va_list ap;
    char* allocBuf;
    va_start(ap, message);
#ifdef _WIN64
    int nSize = _vasprintf(&allocBuf, message, ap);
#else
    int nSize = vasprintf(&allocBuf, message, ap);
#endif
    va_end(ap);

    if (nSize == 0) {
        return false;
    }

    using namespace logging::trivial;
    src::severity_logger< severity_level > lg;

    //BOOST_LOG_SEV(lg, (boost::log::v2s_mt_nt6::trivial::severity_level)level) << allocBuf;
    //BOOST_LOG_SEV(lg, level) << allocBuf;
    //BOOST_LOG_SEV(lg, (boost::log::v2s_mt_posix::trivial::severity_level)level) << allocBuf;
#ifdef _WIN64
    BOOST_LOG_SEV(lg, (boost::log::v2s_mt_nt6::trivial::severity_level)level) << allocBuf;
#else
    BOOST_LOG_SEV(lg, (boost::log::trivial::severity_level)level) << allocBuf;
#endif

    free(allocBuf);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// init
///////////////////////////////////////////////////////////////////////////////
void init(int level, const char* log_dir, const char* log_filename)
{
    namespace expr = boost::log::expressions;

    char fileName[1024];
#ifdef _WIN64
    sprintf_s(fileName, "%s\\%s", log_dir, log_filename);
#else
    sprintf(fileName, "%s/%s", log_dir, log_filename);
#endif

    logging::add_file_log
    (
        keywords::open_mode = (std::ios::out | std::ios::app),
        keywords::file_name = fileName,
        keywords::rotation_size = 1024 * 1024,
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

    //logging::core::get()->set_filter(logging::trivial::severity >= (boost::log::v2s_mt_nt6::trivial::severity_level)level);
    //logging::core::get()->set_filter(logging::trivial::severity >= level);
    //logging::core::get()->set_filter(logging::trivial::severity >= (boost::log::v2s_mt_posix::trivial::severity_level)level);
    logging::core::get()->set_filter(logging::trivial::severity >= (boost::log::trivial::severity_level)level);

}

#ifdef _WIN64
int _vasprintf(char** strp, const char* fmt, va_list ap) {
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
#endif