#ifndef BOT_HPP
#define BOT_HPP

#include <thread>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

#include <nlohmann/json.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

#include "types.hpp"
#include "handlers.hpp"

namespace asio = boost::asio;

namespace cppbot
{
  class Bot
  {
   public:
    Bot(const std::string& token, std::shared_ptr< handlers::MessageHandler > mh);
    void start();
    void stop();
    types::Message sendMessage(size_t chatId, const std::string& text);
   private:
    std::string token_;
    std::shared_ptr< handlers::MessageHandler > mh_;
    asio::io_context ioContext_;
    std::thread ioThread_;
    std::queue< types::Message > messageQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    bool isRunning_ = false;
    asio::ssl::context sslContext_;

    void runIoContext();
    void fetchUpdates();
    void processMessagesAsync();
  };
}

#endif
