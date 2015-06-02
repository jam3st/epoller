#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <vector>

namespace Sb {
      class Logger final {
            friend class Engine;

      public:
            enum class LogType {
                  NOTHING = 0, CRITICAL, ERROR, WARNING, DEBUG, EVERYTHING
            };
            static void logCritical(const std::string&what);
            static void logError(const std::string&what);
            static void logWarn(const std::string&what);
            static void logDebug(const std::string&what);
            static void log(const LogType type, const std::string what);
            static void setMask(const LogType mask);
      private:
            Logger();
            void doLog(LogType type, const std::string what);
            void doSetMask(const LogType level);
            static void start();
            static void stop();
      private:
            std::vector<std::thread::id> threadIds;
            std::mutex lock;
            LogType level = LogType::EVERYTHING;
            static Logger* theLogger;
      };

      const auto logCritical = Logger::logCritical;
      const auto logError = Logger::logError;
      const auto logWarn = Logger::logWarn;
      const auto logDebug = Logger::logDebug;
}
