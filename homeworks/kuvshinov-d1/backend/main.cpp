#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <exprtk.hpp> 

using boost::asio::ip::tcp;
using namespace std::placeholders;

class SimpleWebServer {
public:
    SimpleWebServer(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    tcp::acceptor acceptor_;

    void start_accept() {
        auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
            if (!ec) {
                handle_request(socket);
            }
            start_accept();
        });
    }

    void handle_request(std::shared_ptr<tcp::socket> socket) {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        boost::asio::async_read_until(*socket, *buffer, "\r\n\r\n",
            [this, socket, buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                if (!ec) {
                    std::istream request_stream(buffer.get());
                    std::string request_line;
                    std::getline(request_stream, request_line);

                    if (request_line.find("POST") == 0) {
                        std::ostringstream request_body;
                        request_body << request_stream.rdbuf();
                        std::string expression = request_body.str();

                        // обработка и вычисление
                        std::string response = calculate_expression(expression);

                        // отправка
                        std::string http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + response;
                        boost::asio::async_write(*socket, boost::asio::buffer(http_response), [](const boost::system::error_code&, std::size_t) {});
                    }
                }
            });
    }

    std::string calculate_expression(const std::string& expression_str) {
        using namespace exprtk;

        symbol_table<double> symbol_table;
        double x = 0, y = 0;
        symbol_table.add_variable("x", x);
        symbol_table.add_variable("y", y);
        symbol_table.add_constants();

        expression<double> expression;
        expression.register_symbol_table(symbol_table);

        parser<double> parser;
        if (!parser.compile(expression_str, expression)) {
            return "Error: Failed to parse expression.";
        }

        std::ostringstream result;
        for (x = -10; x <= 10; ++x) {
            for (y = -10; y <= 10; ++y) {
                double calculated_value = expression.value();
                result << "x = " << x << ", y = " << y << " -> " << calculated_value << "\n";
            }
        }

        return result.str();
    }
};

int main() {
    try {
        int port = 8080;
        boost::asio::io_context io_context;
        SimpleWebServer server(io_context, port);
        std::cout << "Запущен на порту "<< port << "\n";
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}