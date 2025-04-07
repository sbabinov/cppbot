#include "bot.hpp"
#include <iostream>
#include "types.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

cppbot::Bot::Bot(const std::string& token, std::shared_ptr< handlers::MessageHandler > mh):
  token_(token),
  mh_(mh),
  sslContext_(asio::ssl::context::tlsv12_client)
{
  sslContext_.set_default_verify_paths();
}

void cppbot::Bot::start() {
  isRunning_ = true;
  ioThread_ = std::thread(&cppbot::Bot::runIoContext, this);
  std::thread([this] { fetchUpdates(); }).detach();
  processMessagesAsync();
}

void cppbot::Bot::stop() {
    isRunning_ = false;
    ioContext_.stop();
    if (ioThread_.joinable()) {
        ioThread_.join();
    }
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
    std::string path = "/bot" + token_ + "/getUpdates?offset=" + std::to_string(lastUpdateId + 1);

    asio::ip::tcp::resolver resolver(ioContext_);
    asio::ssl::stream<asio::ip::tcp::socket> socket(ioContext_, sslContext_);

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
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
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
          std::lock_guard< std::mutex > lock(queueMutex_);
          messageQueue_.push(update["message"].template get< types::Message >());
          queueCondition_.notify_one();
        }
      }
    }
    catch (std::exception const& e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

void cppbot::Bot::processMessagesAsync()
{
  while (isRunning_)
  {
    std::unique_lock<std::mutex> lock(queueMutex_);
    queueCondition_.wait(lock, [this] { return !messageQueue_.empty(); });
    types::Message message = messageQueue_.front();
    messageQueue_.pop();
    lock.unlock();

    std::cout << "Received: " << message.text << std::endl;
  }
}
