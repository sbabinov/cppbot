#ifndef BOT_HPP
#define BOT_HPP

#include <thread>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <utility>

#include <nlohmann/json.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

#include "types.hpp"
#include "handlers.hpp"
#include "states.hpp"

namespace asio = boost::asio;
namespace http = boost::beast::http;

namespace cppbot
{
  class Bot
  {
   public:
    Bot(const std::string& token, std::shared_ptr< handlers::MessageHandler > mh,
      std::shared_ptr< handlers::CallbackQueryHandler > qh, std::shared_ptr< states::Storage > storage);
    void start();
    void stop();

    types::Message sendMessage        (size_t chatId, const std::string& text,
      const types::InlineKeyboardMarkup replyMarkup = types::InlineKeyboardMarkup());
    types::Message editMessageText    (size_t chatId, size_t messageId, const std::string& text);
    bool           deleteMessage      (size_t chatId, size_t messageId);
    bool           answerCallbackQuery(size_t queryId, const std::string& text = "", bool showAlert = false,
      const std::string& url = "", size_t cacheTime = 0);
    types::Message sendPhoto          (size_t chatId, const types::InputFile& photo, const std::string& caption = "",
      const types::InlineKeyboardMarkup& replyMarkup = types::InlineKeyboardMarkup(), bool hasSpoiler = false);
   private:
    std::string token_;
    std::shared_ptr< handlers::MessageHandler > mh_;
    std::shared_ptr< handlers::CallbackQueryHandler > qh_;
    asio::io_context ioContext_;
    asio::ssl::context sslContext_;
    std::thread ioThread_;

    std::queue< types::Message > messageQueue_;
    std::mutex messageQueueMutex_;
    std::condition_variable messageQueueCondition_;

    std::queue< types::CallbackQuery > queryQueue_;
    std::mutex queryQueueMutex_;
    std::condition_variable queryQueueCondition_;

    bool isRunning_ = false;

    states::StateMachine stateMachine_;

    void runIoContext();
    void fetchUpdates();
    http::response< http::string_body > sendRequest(const std::string& body, const std::string& endpoint,
      const std::vector< std::pair < http::field, std::string > >& additionalHeaders = {}, const std::string& contentType = "application/json");
    void processMessages();
    void processCallbackQueries();
  };
}

#endif
