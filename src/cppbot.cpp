#include "cppbot/cppbot.hpp"
#include <iostream>
#include <functional>
#include <future>
#include <utility>
#include "cppbot/types.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

std::string generateBoundary()
{
  return "----CppbotBoundary" + std::to_string(rand());
}

std::string createMultipartBody(const std::string& boundary, const nlohmann::json& fields,
  const std::string& formDataName, const types::InputFile& file, bool isEditing = false)
{
  std::string body = "";

  for (const auto& [field, value] : fields.items())
  {
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"" + field + "\"\r\n\r\n";
    body += value.dump() + "\r\n";
  }

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"" + formDataName + "\"; filename=\"" + file.name() + "\"\r\n";
  body += "Content-type: application/octet-stream\r\n\r\n";
  body += file.asStringBytes() + "\r\n";

  body += "--" + boundary + "--\r\n";
  return body;
}

cppbot::Bot::Bot(const std::string& token, std::shared_ptr< handlers::MessageHandler > mh,
 std::shared_ptr< handlers::CallbackQueryHandler > qh, std::shared_ptr< states::Storage > storage):
  token_(token),
  mh_(mh),
  qh_(qh),
  sslContext_(asio::ssl::context::tlsv12_client),
  stateMachine_(storage),
  isRunning_(false)
{
  sslContext_.set_default_verify_paths();
}

void cppbot::Bot::startPolling()
{
  isRunning_ = true;
  ioThread_ = std::thread(&cppbot::Bot::runIoContext, this);
  std::thread(std::bind(&cppbot::Bot::fetchUpdates, this)).detach();
  processUpdates();
}

void cppbot::Bot::stop()
{
  isRunning_ = false;
  ioContext_.stop();
  if (ioThread_.joinable())
  {
    ioThread_.join();
  }
}

cppbot::Bot::futureMessage cppbot::Bot::sendMessage(size_t chatId, const std::string& text,
  const types::Keyboard& replyMarkup)
{
  nlohmann::json body = {
    {"chat_id", chatId},
    {"text", text}
  };
  nlohmann::json jsonReplyMarkup = replyMarkup.toJson();
  if (!jsonReplyMarkup.empty())
  {
    body["reply_markup"] = replyMarkup.toJson();
  }
  return sendRequest< types::Message >(body.dump(), "/sendMessage");
}

cppbot::Bot::futureBool cppbot::Bot::deleteMessage(size_t chatId, size_t messageId)
{
  nlohmann::json body = {
    {"chat_id", chatId},
    {"message_id", messageId}
  };
  return sendRequest< bool >(body.dump(), "/deleteMessage");
}

cppbot::Bot::futureBool cppbot::Bot::answerCallbackQuery(const std::string& queryId, const std::string& text,
  bool showAlert, const std::string& url, size_t cacheTime)
{
  nlohmann::json body = {
    {"callback_query_id", queryId},
    {"show_alert", showAlert},
    {"cache_time", cacheTime}
  };
  if (text != "")
  {
    body["text"] = text;
  }
  if (url != "")
  {
    body["url"] = url;
  }
  return sendRequest< bool >(body.dump(), "/answerCallbackQuery");
}

cppbot::Bot::futureMessage cppbot::Bot::sendPhoto(size_t chatId, const types::InputFile& photo,
const std::string& caption, const types::InlineKeyboardMarkup& replyMarkup, bool hasSpoiler)
{
  std::string boundary = generateBoundary();
  nlohmann::json fields;
  fields["chat_id"] = chatId;
  if (caption != "")
  {
    fields["caption"] = caption;
  }
  if (!replyMarkup.keyboard.empty())
  {
    fields["reply_markup"] = replyMarkup;
  }
  if (hasSpoiler)
  {
    fields["has_spoiler"] = hasSpoiler;
  }
  return sendFile(photo, "/sendPhoto", fields);
}

cppbot::Bot::futureMessage cppbot::Bot::sendDocument(size_t chatId, const types::InputFile& document,
  const std::string& caption, const types::InlineKeyboardMarkup& replyMarkup)
{
  std::string boundary = generateBoundary();
  nlohmann::json fields;
  fields["chat_id"] = chatId;
  if (caption != "")
  {
    fields["caption"] = caption;
  }
  if (!replyMarkup.keyboard.empty())
  {
    fields["reply_markup"] = replyMarkup;
  }
  return sendFile(document, "/sendDocument", fields);
}

cppbot::Bot::futureMessage cppbot::Bot::sendAudio(size_t chatId, const types::InputFile& audio,
  const std::string& caption, const types::InlineKeyboardMarkup& replyMarkup)
{
  std::string boundary = generateBoundary();
  nlohmann::json fields;
  fields["chat_id"] = chatId;
  if (caption != "")
  {
    fields["caption"] = caption;
  }
  if (!replyMarkup.keyboard.empty())
  {
    fields["reply_markup"] = replyMarkup;
  }
  return sendFile(audio, "/sendAudio", fields);
}

cppbot::Bot::futureMessage cppbot::Bot::sendVideo(size_t chatId, const types::InputFile& video,
  const std::string& caption, const types::InlineKeyboardMarkup& replyMarkup, bool hasSpoiler)
{
  std::string boundary = generateBoundary();
  nlohmann::json fields;
  fields["chat_id"] = chatId;
  if (caption != "")
  {
    fields["caption"] = caption;
  }
  if (!replyMarkup.keyboard.empty())
  {
    fields["reply_markup"] = replyMarkup;
  }
  if (hasSpoiler)
  {
    fields["has_spoiler"] = hasSpoiler;
  }
  return sendFile(video, "/sendVideo", fields);
}

cppbot::Bot::futureMessage cppbot::Bot::editMessageText(size_t chatId, size_t messageId, const std::string& text)
{
  nlohmann::json body = {
    {"chat_id", chatId},
    {"message_id", messageId},
    {"text", text}
  };
  return sendRequest< types::Message >(body.dump(), "/editMessageText");
}

cppbot::Bot::futureMessage cppbot::Bot::editMessageCaption(size_t chatId, size_t messageId,
  const std::string& caption, const types::InlineKeyboardMarkup& replyMarkup)
{
  nlohmann::json body = {
    {"chat_id", chatId},
    {"message_id", messageId},
    {"caption", caption}
  };
  if (!replyMarkup.keyboard.empty())
  {
    body["reply_markup"] = replyMarkup;
  }

  return sendRequest< types::Message >(body.dump(), "/editMessageCaption");
}

cppbot::Bot::futureMessage cppbot::Bot::editMessageMedia(size_t chatId, size_t messageId, const types::InputMedia& media,
  const types::InlineKeyboardMarkup& replyMarkup)
{
  std::string boundary = generateBoundary();
  nlohmann::json fields;
  fields["chat_id"] = chatId;
  fields["message_id"] = messageId;
  fields["media"] = media;
  if (!replyMarkup.keyboard.empty())
  {
    fields["reply_markup"] = replyMarkup;
  }
  return updateFile(media, fields);
}

cppbot::Bot::futureMessage cppbot::Bot::editMessageReplyMarkup(size_t chatId, size_t messageId,
  const types::InlineKeyboardMarkup& replyMarkup)
{
  nlohmann::json body = {
    {"chat_id", chatId},
    {"message_id", messageId},
    {"reply_markup", replyMarkup}
  };
  return sendRequest< types::Message >(body.dump(), "/editMessageReplyMarkup");
}

cppbot::Bot::futureFile cppbot::Bot::getFile(const std::string& fileId)
{
  nlohmann::json body = {
    {"file_id", fileId}
  };
  return sendRequest< types::File >(body.dump(), "/getFile");
}

states::StateContext cppbot::Bot::getStateContext(size_t chatId)
{
  return states::StateContext(chatId, &stateMachine_);
}

void cppbot::Bot::runIoContext()
{
  asio::executor_work_guard< asio::io_context::executor_type > work = asio::make_work_guard(ioContext_);
  ioContext_.run();
}

void cppbot::Bot::fetchUpdates()
{
  std::size_t lastUpdateId = 0;
  while (isRunning_)
  {
    std::string host = "api.telegram.org";
    std::string path = "/bot" + token_ + "/getUpdates?timeout=30&offset=" + std::to_string(lastUpdateId + 1);

    asio::ip::tcp::resolver resolver(ioContext_);
    asio::ssl::stream< asio::ip::tcp::socket > socket(ioContext_, sslContext_);

    try
    {
      if (!SSL_set_tlsext_host_name(socket.native_handle(), host.c_str()))
      {
        throw boost::system::system_error(::ERR_get_error(), asio::error::get_ssl_category());
      }

      auto const results = resolver.resolve(host, "443");
      asio::connect(socket.next_layer(), results.begin(), results.end());

      socket.handshake(asio::ssl::stream_base::client);

      http::request< http::string_body > req{http::verb::get, path, 11};
      req.set(http::field::host, host);
      req.set(http::field::user_agent, "CppBot/1.0");

      http::write(socket, req);

      beast::flat_buffer buffer;
      http::response< http::string_body > res;
      http::read(socket, buffer, res);
      auto updates = nlohmann::json::parse(res.body());
      for (const auto& update : updates["result"])
      {
        lastUpdateId = update["update_id"];
        if (update.contains("message"))
        {
          std::lock_guard< std::mutex > lock(updateMutex_);
          messageQueue_.push(update["message"].template get< types::Message >());
          updateCondition_.notify_one();
        }
        if (update.contains("callback_query"))
        {
          std::lock_guard< std::mutex > lock(updateMutex_);
          queryQueue_.push(update["callback_query"].template get< types::CallbackQuery >());
          updateCondition_.notify_one();
        }
      }
    }
    catch (std::exception const& e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

cppbot::Bot::futureMessage cppbot::Bot::sendFile(const types::InputFile& file, const std::string& endpoint,
  const nlohmann::json& fields)
{
  std::string fileType = "";
  if (endpoint == "/sendPhoto")
  {
    fileType = "photo";
  }
  else if (endpoint == "/sendDocument")
  {
    fileType = "document";
  }
  else if (endpoint == "/sendAudio")
  {
    fileType = "audio";
  }
  else if (endpoint == "/sendVideo")
  {
    fileType = "video";
  }

  std::string boundary = generateBoundary();
  std::string body = createMultipartBody(boundary, fields, fileType, file);
  return sendRequest< types::Message >(
    body,
    endpoint,
    {{http::field::content_length, std::to_string(body.size())}, {http::field::connection, "close"}},
    "multipart/form-data; boundary=" + boundary
  );
}

cppbot::Bot::futureMessage cppbot::Bot::updateFile(const types::InputMedia& media, const nlohmann::json& fields)
{
  std::string boundary = generateBoundary();

  std::string body = createMultipartBody(boundary, fields, media.file().name(), media.file());
  return sendRequest< types::Message >(
    body,
    "/editMessageMedia",
    {{http::field::content_length, std::to_string(body.size())}, {http::field::connection, "close"}},
    "multipart/form-data; boundary=" + boundary
  );
}

void cppbot::Bot::printError(const std::string& errorMessage) const
{
  std::cerr << "Request Error: " << errorMessage << '\n';
}

void cppbot::Bot::processUpdates()
{
  while (isRunning_)
  {
    std::unique_lock< std::mutex > lock(updateMutex_);
    updateCondition_.wait(lock, [this]
    {
      return !messageQueue_.empty() || !queryQueue_.empty();
    });
    try
    {
      if (!messageQueue_.empty())
      {
        types::Message msg = messageQueue_.front();
        messageQueue_.pop();
        lock.unlock();
        states::StateContext state(msg.chat.id, &stateMachine_);
        (*mh_).processMessage(msg, state);
      }
      else
      {
        types::CallbackQuery query = queryQueue_.front();
        queryQueue_.pop();
        lock.unlock();
        (*qh_).processCallbackQuery(query);
      }
    }
    catch (const std::exception& e)
    {
      std::cerr << e.what() << '\n';
    }
  }
}
