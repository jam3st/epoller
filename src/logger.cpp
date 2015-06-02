#include <sstream>
#include "logger.hpp"
#include "utils.hpp"
#include "clock.hpp"

namespace Sb {
      Logger* Logger::theLogger = nullptr;

      Logger::Logger() {
      }

      void Logger::start() {
            if (Logger::theLogger != nullptr) {
                  throw std::runtime_error("start called when already started");
            }
            Logger::theLogger = new Logger;
      }

      void Logger::setMask(const LogType mask) {
            Logger::theLogger->doSetMask(mask);
      }

      void Logger::doSetMask(const LogType level) {
            this->level = level;
      }

      void Logger::stop() {
            if (Logger::theLogger == nullptr) {
                  throw std::runtime_error("stop called before start");
            }
            std::unique_ptr<Logger> tmp(Logger::theLogger);
            Logger::theLogger = nullptr;
      }

      void Logger::logCritical(const std::string&what) {
            Logger::log(LogType::CRITICAL, what);
      }

      void Logger::logError(const std::string&what) {
            Logger::log(LogType::ERROR, what);
      }

      void Logger::logWarn(const std::string&what) {
            Logger::log(LogType::WARNING, what);
      }

      void Logger::logDebug(const std::string&what) {
            Logger::log(LogType::DEBUG, what);
      }

      void Logger::log(const LogType type, const std::string what) {
            if (Logger::theLogger == nullptr) {
                  throw std::runtime_error("need to call start first or start failed");
            }
            Logger::theLogger->doLog(type, what);
      }

      void Logger::doLog(LogType type, const std::string what) {
            if (type > level) {
                  return;
            }
            int64_t nowNs = Clock::now();
            std::time_t secs = nowNs / ONE_SEC_IN_NS;
            int64_t nsecs = nowNs % ONE_SEC_IN_NS;
            lock.lock();
            std::tm* local_time_now = std::localtime(&secs);
            lock.unlock();
            std::locale loc;
            const std::time_put<char>&tmput{std::use_facet < std::time_put<char>>(loc)};
            std::stringbuf line;
            std::ostream os(&line);
            std::string fmt("%Y-%Om-%Od %OH:%OM:%OS.");
            tmput.put(os, os, '.', local_time_now, fmt.data(), fmt.data() + fmt.length());
            auto start = nsecs == 0 ? static_cast<int64_t>(1) : nsecs;
            for (auto div = start; div < 100000000; div = div * 10) {
                  os << "0";
            }
            os << nsecs;
            lock.lock();
            std::size_t tid = std::numeric_limits<std::size_t>::max();
            for (std::size_t i = 0; i < Logger::threadIds.size(); ++i) {
                  if (threadIds.at(i) == std::this_thread::get_id()) {
                        tid = i;
                  }
            }
            if (tid == std::numeric_limits<std::size_t>::max()) {
                  Logger::threadIds.push_back(std::this_thread::get_id());
                  tid = Logger::threadIds.size() - 1;
            }
            lock.unlock();
            os << " " << std::setw(2) << tid << " ";
            os << what << "\n";
            lock.lock();
            std::cerr << line.str();
            lock.unlock();
      }
}
