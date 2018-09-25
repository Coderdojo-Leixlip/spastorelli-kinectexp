#include <iostream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

class WsServer {
public:
    WsServer() {
        s.set_error_channels(websocketpp::log::elevel::all);
        s.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

        s.init_asio();
        s.set_message_handler(std::bind(
            &WsServer::on_message, this,
            std::placeholders::_1, std::placeholders::_2
        ));
    }
    
    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
        const std::string msg_str = msg->get_payload();
        s.send(hdl, msg_str, msg->get_opcode());
    }

    void run() {
        s.listen(port);
        s.start_accept();
        std::cout << "Listening on port " << port << "..." << std::endl;
        s.run();
    }

private:
    server s;
    const int port = 9002;
};

int main() {
    WsServer s;
    s.run();
}