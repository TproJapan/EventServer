#pragma once
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

#define LOG_DIR_SERV "c:\\tmp"
#define LOG_FILENAME_SERV "boostlog_%N.log"

//level;0(trace),1(debug),2(info),3(warning),4(error),5(fatal)
extern void init(int level, const char* log_dir, const char* log_filename);
extern void write_log(int level, char* message);
extern void test1();
extern void test2();