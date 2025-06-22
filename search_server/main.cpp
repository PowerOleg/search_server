
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include "file_manager.h"
#include "postgres_manager.h"
#include "thread_pool.h"

#pragma execution_character_set("utf-8")

namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;         // from <boost/beast/websocket.hpp>
namespace net = boost::asio;                    // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

struct Config
{
    std::string sqlhost;//хост, на котором запущена база данных;
    std::string sqlport;//порт, на котором запущена база данных;
    std::string dbname;//название базы данных;
    std::string username;//имя пользователя для подключения к базе данных;
    std::string password;//пароль пользователя для подключения к базе данных;
    std::string url;//стартовая страница для программы «Паук»;
    std::string crawler_depth;//глубина рекурсии для программы «Паук»;
    std::string http_port;//порт для запуска программы - поисковика.
};
Config config;

std::string ProcessHttpPostRequest(const std::string &request_body);
std::vector<std::string> ParseBody(const std::string& request_body);
std::string MakeResponse(const std::string& request_body, const std::vector<std::string>& words, const std::map<std::string, std::vector<std::pair<std::string, int>>> &document_word_quantity);

// Return a reasonable mime type based on the extension of a file.
beast::string_view mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if (pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();
    if (iequals(ext, ".htm"))  return "text/html";
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".php"))  return "text/html";
    if (iequals(ext, ".css"))  return "text/css";
    if (iequals(ext, ".txt"))  return "text/plain";
    if (iequals(ext, ".js"))   return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".xml"))  return "application/xml";
    if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))  return "video/x-flv";
    if (iequals(ext, ".png"))  return "image/png";
    if (iequals(ext, ".jpe"))  return "image/jpeg";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg"))  return "image/jpeg";
    if (iequals(ext, ".gif"))  return "image/gif";
    if (iequals(ext, ".bmp"))  return "image/bmp";
    if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff")) return "image/tiff";
    if (iequals(ext, ".tif"))  return "image/tiff";
    if (iequals(ext, ".svg"))  return "image/svg+xml";
    if (iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path)
{
    if (base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto& c : result)
        if (c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator handle_request(beast::string_view doc_root, http::request<Body, http::basic_fields<Allocator>>&& req)
{
    // Returns a bad request response
    auto const bad_request = [&req](beast::string_view why)
        {
            http::response<http::string_body> res{ http::status::bad_request, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        };

    // Returns a not found response
    auto const not_found = [&req](beast::string_view target)
        {
            http::response<http::string_body> res{ http::status::not_found, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        };

    // Returns a server error response
    auto const server_error = [&req](beast::string_view what)
        {
            http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();
            return res;
        };



    if (req.method() == http::verb::post && req.target() == "/search")
    {
        std::string request_value = req.body().substr(6);
        std::string response_body = ProcessHttpPostRequest(request_value);

        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(response_body);
        res.prepare_payload();

        return res;
        /*auto it3 = req.find("first");
        std::string first = it3->name_string();
        std::cout << "find first:" << first << std::endl;*/
    }


    // Make sure we can handle the method
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return bad_request("Unknown HTTP-method");

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos)
        return bad_request("Illegal request-target");



    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == beast::errc::no_such_file_or_directory)
        return not_found(req.target());

    // Handle an unknown error
    if (ec)
        return server_error(ec.message());

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version()) };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class websocket_session : public std::enable_shared_from_this<websocket_session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

public:
    // Take ownership of the socket
    explicit websocket_session(tcp::socket&& socket)
        : ws_(std::move(socket))
    {}

    // Start the asynchronous accept operation
    template<class Body, class Allocator>
    void do_accept(http::request<Body, http::basic_fields<Allocator>> req)
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                    " advanced-server");
            }));

        // Accept the websocket handshake
        ws_.async_accept(
            req,
            beast::bind_front_handler(
                &websocket_session::on_accept,
                shared_from_this()));
    }

private:
    void on_accept(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &websocket_session::on_read,
                shared_from_this()));
    }

    void on_read(
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the websocket_session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec)
            fail(ec, "read");

        // Echo the message
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()));
    }

    void on_write(
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
};

//------------------------------------------------------------------------------

// Handles an HTTP server connection
class http_session : public std::enable_shared_from_this<http_session>
{
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;

    static constexpr std::size_t queue_limit = 8; // max responses
    std::queue<http::message_generator> response_queue_;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;

public:
    // Take ownership of the socket
    http_session(tcp::socket&& socket, std::shared_ptr<std::string const> const& doc_root)
        : stream_(std::move(socket)), doc_root_(doc_root)
    {
        static_assert(queue_limit > 0, "queue limit must be positive");
    }

    // Start the session
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&http_session::do_read, this->shared_from_this()));
    }

private:
    void do_read()
    {
        // Construct a new parser for each message
        parser_.emplace();

        // Apply a reasonable limit to the allowed size
        // of the body in bytes to prevent abuse.
        parser_->body_limit(10000);

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request using the parser-oriented interface
        http::async_read(stream_, buffer_, *parser_, beast::bind_front_handler(&http_session::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (ec == http::error::end_of_stream)
            return do_close();

        if (ec)
            return fail(ec, "read");

        // See if it is a WebSocket Upgrade
        if (websocket::is_upgrade(parser_->get()))
        {
            // Create a websocket session, transferring ownership
            // of both the socket and the HTTP request.
            std::make_shared<websocket_session>(stream_.release_socket())->do_accept(parser_->release());
            return;
        }

        // Send the response
        queue_write(handle_request(*doc_root_, parser_->release()));

        // If we aren't at the queue limit, try to pipeline another request
        if (response_queue_.size() < queue_limit)
            do_read();
    }

    void queue_write(http::message_generator response)
    {
        // Allocate and store the work
        response_queue_.push(std::move(response));

        // If there was no previous work, start the write loop
        if (response_queue_.size() == 1)
            do_write();
    }

    // Called to start/continue the write-loop. Should not be called when
    // write_loop is already active.
    void do_write()
    {
        if (!response_queue_.empty())
        {
            bool keep_alive = response_queue_.front().keep_alive();

			beast::async_write(stream_, std::move(response_queue_.front()), beast::bind_front_handler(&http_session::on_write,
					shared_from_this(),	keep_alive));
        }
    }

    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        if (!keep_alive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Resume the read if it has been paused
        if (response_queue_.size() == queue_limit)
            do_read();

        response_queue_.pop();

        do_write();
    }

    void do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<std::string const> doc_root_;

public:
    listener(net::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<std::string const> const& doc_root)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)), doc_root_(doc_root)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(acceptor_.get_executor(), beast::bind_front_handler(&listener::do_accept, this->shared_from_this()));
    }

private:
    void do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(&listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the http session and run it
            std::make_shared<http_session>(std::move(socket), doc_root_)->run();
        }

        // Accept another connection
        do_accept();
    }
};

int main(int argc, char* argv[])
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    File_manager file_manager("config.ini");
    file_manager.FillConfig(
        &config.sqlhost,
        &config.sqlport,
        &config.dbname,
        &config.username,
        &config.password,
        &config.url,
        &config.crawler_depth,
        &config.http_port);

    auto const address = net::ip::make_address(config.sqlhost);
    auto const port = static_cast<unsigned short>(std::atoi(config.http_port.c_str()));
    auto const doc_root = std::make_shared<std::string>(".");
    const size_t thread_quantity = 1;
    net::io_context ioc{ thread_quantity };
    Thread_pool thread_pool(ioc, thread_quantity);
    
    std::make_shared<listener>(ioc, tcp::endpoint{ address, port }, doc_root)->run();// Create and launch a listening port
    net::signal_set signals(ioc, SIGINT, SIGTERM);// Capture SIGINT and SIGTERM to perform a clean shutdown
    signals.async_wait(
        [&](beast::error_code const&, int)
        {
            ioc.stop();// Stop the `io_context`. This will cause `run()` to return immediately, eventually destroying the `io_context` and all of the sockets in it.
        });

    thread_pool.JoinAll();
    return EXIT_SUCCESS;
}

std::string ProcessHttpPostRequest(const std::string &request_body)
{
    Postgres_manager postgres(config.sqlhost, config.sqlport, config.dbname, config.username, config.password);
    std::vector<std::string> words = ParseBody(request_body);
    std::map<std::string, std::vector<std::pair<std::string, int>>> document_words_quantity = postgres.FindWordsOccurrance(words);
    std::string response = MakeResponse(request_body, words, document_words_quantity);
    
    return response;
}

std::vector<std::string> ParseBody(const std::string& request_body)
{
    std::vector<std::string> words;
    std::stringstream stream(request_body);
    std::string buffer = "";
    std::cout << "PARSING REQUEST BODY: " << std::endl;
    while (getline(stream, buffer, '+')) {
        std::cout << buffer << std::endl;
        words.push_back(buffer);
    };
    return words;
}

std::string MakeResponse(const std::string& request_body, const std::vector<std::string> &words, const std::map<std::string, std::vector<std::pair<std::string, int>>> &document_word_quantity_map)
{
    std::string response = "";
    std::map <int, std::string> urls;

    if (document_word_quantity_map.size() == 0 /* || document_word_quantity_map.at(0).first == "502" */)
    {
        response = "<h1>Nothing was found</h1>\n";
    }
    else
    {
        for (const auto& document_word_quantity : document_word_quantity_map)
        {
            //std::pair<std::string, int> word_quantity = document_word_quantity.second;

        }
       







        response += "<h1>Top 10 search results of word: " + request_body + "</h1>\n";
        int count = 0;
        for (const auto& key_value : urls)//auto it = map.begin(); it != map.end(); ++it 
        {
            response += "<p>" + std::to_string(++count) + ". " + key_value.second + "</p>" + "\n";
        }
    }
    

    return response;
}
