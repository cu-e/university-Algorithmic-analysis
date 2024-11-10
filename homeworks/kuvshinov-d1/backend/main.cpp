#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <exprtk.hpp>
#include <json/json.h>

using boost::asio::ip::tcp;
using namespace std::placeholders;

// интерфейс для обработки запросов
class IRequestHandler {
public:
    virtual void handle_request(std::shared_ptr<tcp::socket> socket, const std::string& expression, double x_min, double x_max, double x_step) = 0;
    virtual ~IRequestHandler() = default;
};

// обработчик пост запроса
class CalculationRequestHandler : public IRequestHandler {
public:
    void handle_request(std::shared_ptr<tcp::socket> socket, const std::string& expression, double x_min, double x_max, double x_step) override {
        std::string response = calculate_expression(expression, x_min, x_max, x_step);

        std::string http_response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, Expression, X-Min, X-Max, X-Step\r\n\r\n" + response;
        
        boost::asio::async_write(*socket, boost::asio::buffer(http_response),
            [socket](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    std::cerr << "Ошибка отправки ответа: " << ec.message() << std::endl;
                }
            });
    }

private:
    std::string calculate_expression(const std::string& expression_str, double x_min, double x_max, double x_step) {
        using namespace exprtk;

        symbol_table<double> symbol_table;
        double x = 0;
        symbol_table.add_variable("x", x);
        symbol_table.add_constants();

        expression<double> expression;
        expression.register_symbol_table(symbol_table);

        parser<double> parser;
        if (!parser.compile(expression_str, expression)) {
            std::string error_message = "Ошибка при разборе выражения!\n";
            error_message += "Исключение: " + expression_str + "\n";
            error_message += "Ошибка: " + parser.error();
            return error_message;
        }

        // json форматер
        Json::Value json_response;
        for (x = x_min; x <= x_max; x += x_step) {
            double calculated_value = expression.value();
            Json::Value data_point;
            data_point["x"] = x;
            data_point["value"] = calculated_value;
            json_response.append(data_point);
        }

        Json::StreamWriterBuilder writer;
        return Json::writeString(writer, json_response);
    }
};

// обработчик ошибок
class ErrorRequestHandler : public IRequestHandler {
public:
    void handle_request(std::shared_ptr<tcp::socket> socket, const std::string& expression, double x_min, double x_max, double x_step) override {
        std::string error_message = "Ошибка: не указано выражение!"; 

        std::string http_response = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, Expression, X-Min, X-Max, X-Step\r\n\r\n" + error_message;
        
        boost::asio::async_write(*socket, boost::asio::buffer(http_response),
            [socket](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    std::cerr << "Ошибка при отправке ответа с ошибкой: " << ec.message() << std::endl;
                }
            });
    }
};

// работа с http запросами
class HttpRequestParser {
public:
    // FIXME: Сделать получение через тело запроса, тк пока что через хедеры (нет понятия почему не могу считать тело запроса, проверить управляющие символы) 
    bool parse_request(std::istream& request_stream, std::string& expression, double& x_min, double& x_max, double& x_step) {
        std::string header_line;
        while (std::getline(request_stream, header_line) && header_line != "\r") {
            if (header_line.find("Expression:") == 0) {
                expression = header_line.substr(11);  
            }
            if (header_line.find("X-Min:") == 0) {
                x_min = std::stod(header_line.substr(7));
            }
            if (header_line.find("X-Max:") == 0) {
                x_max = std::stod(header_line.substr(7));
            }
            if (header_line.find("X-Step:") == 0) {
                x_step = std::stod(header_line.substr(8));
            }
        }

        return !expression.empty();
    }
};

// веб - сервер
class WebServer {
public:
    WebServer(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), 
          calculation_handler_(std::make_shared<CalculationRequestHandler>()), 
          error_handler_(std::make_shared<ErrorRequestHandler>()), 
          parser_(std::make_shared<HttpRequestParser>()) {
        start_accept();
    }

private:
    tcp::acceptor acceptor_;
    std::shared_ptr<IRequestHandler> calculation_handler_;
    std::shared_ptr<IRequestHandler> error_handler_;
    std::shared_ptr<HttpRequestParser> parser_;

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

                    std::istringstream request_line_stream(request_line);
                    std::string method, path, version;
                    request_line_stream >> method >> path >> version;

                    if (request_line.find("OPTIONS") == 0) {
                        // CORS
                        std::string http_response = "HTTP/1.1 200 OK\r\n"
                                                    "Access-Control-Allow-Origin: *\r\n"
                                                    "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                                                    "Access-Control-Allow-Headers: Content-Type, Expression, X-Min, X-Max, X-Step\r\n\r\n";
                        boost::asio::async_write(*socket, boost::asio::buffer(http_response),
                            [socket](const boost::system::error_code& ec, std::size_t) {
                                if (ec) {
                                    std::cerr << "Ошибка отправки ответа на OPTIONS: " << ec.message() << std::endl;
                                }
                            });
                    } else if (request_line.find("POST") == 0 && path.find("/api/")) {
                        std::string expression;
                        double x_min = -10, x_max = 10, x_step = 1;

                        // парсинг тела запроса
                        if (parser_->parse_request(request_stream, expression, x_min, x_max, x_step)) {
                            calculation_handler_->handle_request(socket, expression, x_min, x_max, x_step);
                        } else {
                            error_handler_->handle_request(socket, expression, x_min, x_max, x_step);
                        }
                    }
                }
            });
    }
};

int main() {
    try {
        int port = 8088;
        boost::asio::io_context io_context;
        WebServer server(io_context, port);
        std::cout << "Сервер запущен на порту " << port << "\n";
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Исключение: " << e.what() << "\n";
    }
    return 0;
}