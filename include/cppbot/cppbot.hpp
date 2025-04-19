#ifndef BOT_HPP
#define BOT_HPP

#include <iostream>
#include <future>
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
    using futureMessage = std::future< types::Message >;
    using futureFile = std::future< types::File >;
    using futureBool = std::future< bool >;

    Bot(const std::string& token, std::shared_ptr< handlers::MessageHandler > mh,
      std::shared_ptr< handlers::CallbackQueryHandler > qh, std::shared_ptr< states::Storage > storage);
    void startPolling();
    void stop();

    futureMessage sendMessage           (size_t chatId, const std::string& text,
      const types::Keyboard& replyMarkup = {});

    futureBool    deleteMessage        (size_t chatId, size_t messageId);

    futureBool    answerCallbackQuery  (const std::string& queryId, const std::string& text = "",
      bool showAlert = false, const std::string& url = "", size_t cacheTime = 0);

    futureMessage sendPhoto             (size_t chatId, const types::InputFile& photo, const std::string& caption = "",
      const types::InlineKeyboardMarkup& replyMarkup = {}, bool hasSpoiler = false);
    futureMessage sendDocument          (size_t chatId, const types::InputFile& document, const std::string& caption = "",
      const types::InlineKeyboardMarkup& replyMarkup = {});
    futureMessage sendAudio             (size_t chatId, const types::InputFile& audio, const std::string& caption = "",
      const types::InlineKeyboardMarkup& replyMarkup = {});
    futureMessage sendVideo             (size_t chatId, const types::InputFile& video, const std::string& caption = "",
      const types::InlineKeyboardMarkup& replyMarkup = {}, bool hasSpoiler = false);

    futureMessage editMessageText       (size_t chatId, size_t messageId, const std::string& text);
    futureMessage editMessageCaption    (size_t chatId, size_t messageId, const std::string& caption,
      const types::InlineKeyboardMarkup& replyMarkup = {});
    futureMessage editMessageMedia      (size_t chatId, size_t messageId, const types::InputMedia& media,
      const types::InlineKeyboardMarkup& replyMarkup = {});
    futureMessage editMessageReplyMarkup(size_t chatId, size_t messageId,
      const types::InlineKeyboardMarkup& replyMarkup);

    futureFile    getFile               (const std::string& fileId);

    states::StateContext getStateContext(size_t chatId);
   private:
    std::string token_;
    std::shared_ptr< handlers::MessageHandler > mh_;
    std::shared_ptr< handlers::CallbackQueryHandler > qh_;
    asio::io_context ioContext_;
    asio::ssl::context sslContext_;
    std::thread ioThread_;
    std::queue< types::Message > messageQueue_;
    std::queue< types::CallbackQuery > queryQueue_;
    std::mutex updateMutex_;
    std::condition_variable updateCondition_;
    states::StateMachine stateMachine_;
    bool isRunning_;

    void runIoContext();
    void fetchUpdates();
    void processUpdates();

    futureMessage sendFile(const types::InputFile& file, const std::string& fileType,
      const nlohmann::json& fields);
    futureMessage updateFile(const types::InputMedia& media, const nlohmann::json& fields);

    void printError(const std::string& errorMessage) const;

    struct RequestData
    {
      std::string body;
      std::string endpoint;
      std::vector< std::pair< http::field, std::string > > additionalHeaders;
      std::string contentType;
      std::shared_ptr< asio::ip::tcp::resolver > resolver;
      std::shared_ptr < asio::ssl::stream<asio::ip::tcp::socket> > socket;
      std::shared_ptr< http::request< http::string_body > > req;
      std::shared_ptr< http::response<http::string_body> > res;
      std::shared_ptr< boost::beast::flat_buffer > buffer;
    };

    template< typename T >
    std::future< T > sendRequest(const std::string& body, const std::string& endpoint,
      const std::vector< std::pair< http::field, std::string > >& additionalHeaders = {},
      const std::string& contentType = "application/json")
    {
      auto data = std::make_shared< RequestData >();

      data->body = body;
      data->endpoint = endpoint;
      data->additionalHeaders = additionalHeaders;
      data->contentType = contentType;
      data->resolver = std::make_shared<  asio::ip::tcp::resolver >(ioContext_);
      data->socket = std::make_shared< asio::ssl::stream<asio::ip::tcp::socket> >(ioContext_, sslContext_);

      auto promise = std::make_shared< std::promise< T > >();
      std::future< T > future = promise->get_future();

      if (!SSL_set_tlsext_host_name(data->socket->native_handle(), "api.telegram.org"))
      {
        printError("Problems with SSL");
        promise->set_exception(std::make_exception_ptr(std::runtime_error("Correct message wasn't received")));
      }
      data->resolver->async_resolve("api.telegram.org", "443",
        [this, data, promise](auto ec, auto endpoints)
        {
          if (ec)
          {
            printError(ec.message());
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Correct message wasn't received")));
            return;
          }
          async_connect(data->socket->next_layer(), endpoints, [this, data, promise](auto ec, auto)
          {
            if (ec)
            {
              printError(ec.message());
              promise->set_exception(std::make_exception_ptr(std::runtime_error("Correct message wasn't received")));
              return;
            }
            data->socket->async_handshake(asio::ssl::stream_base::client, [this, data, promise](auto ec)
            {
              if (ec)
              {
                printError(ec.message());
                promise->set_exception(std::make_exception_ptr(std::runtime_error("Correct message wasn't received")));
                return;
              }
              std::string host = "api.telegram.org";
              std::string path = "/bot" + token_ + data->endpoint;
              auto req = std::make_shared< http::request< http::string_body > >(http::verb::post, path, 11);
              req->set(http::field::host, host);
              req->set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
              req->set(http::field::content_type, data->contentType);
              for (const auto& header : data->additionalHeaders)
              {
                req->set(header.first, header.second);
              }
              req->body() = data->body;
              req->prepare_payload();
              data->req = req;
              http::async_write(*(data->socket), *(data->req), [this, data, promise](auto ec, auto)
              {
                if (ec)
                {
                  printError(ec.message());
                  promise->set_exception(std::make_exception_ptr(std::runtime_error("Correct message wasn't received")));
                  return;
                }
                auto buffer = std::make_shared< boost::beast::flat_buffer >();
                auto res = std::make_shared< http::response<http::string_body> >();
                data->buffer = buffer;
                data->res = res;
                http::async_read(*(data->socket), *(data->buffer), *(data->res), [this, promise, data](auto ec, auto)
                {
                  if (ec)
                  {
                    printError(ec.message());
                    promise->set_exception(std::make_exception_ptr(std::runtime_error("Correct message wasn't received")));
                    return;
                  }
                  nlohmann::json response = nlohmann::json::parse(data->res->body());
                  if (!response["ok"])
                  {
                    printError(response["description"].dump());
                    promise->set_exception(std::make_exception_ptr(std::runtime_error("Correct message wasn't received")));
                    return;
                  }
                  promise->set_value(response["result"].template get< T >());
                });
              });
            });
          });
        });
      return future;
    }
  };
}

#endif
