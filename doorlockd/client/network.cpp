#include "network.h"

#include "../lib/response.h"

namespace ba = boost::asio;
using ba::ip::tcp;

const std::string NetworkThread::_subscription_command = "{ \"command\": \"subscribe\"}";

NetworkThread::NetworkThread(const std::string &hostname,
		                     const unsigned short port) :
	QThread(),
	_l(Logger::get()),
	_hostname(hostname),
	_port(port),
	_io_service()
{
}

void NetworkThread::run() {
    do {
        try {
            tcp::resolver resolver(_io_service);
            tcp::socket socket(_io_service);
            tcp::resolver::query query(_hostname, std::to_string(_port));
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            tcp::resolver::iterator end;
            boost::system::error_code error = ba::error::host_not_found;
            std::vector<char> data;

            while (error && endpoint_iterator != end) {
                socket.close();
                socket.connect(*endpoint_iterator++, error);
            }
            if (error)
                throw boost::system::system_error(error);

            // After connection is established, send the subscription command
            socket.write_some(ba::buffer(_subscription_command), error);
            if (error)
                throw boost::system::system_error(error);

            data.resize(_SOCKET_BUFFERLENGTH);

            std::function<void(void)> receiveMessage = [&] () {
                socket.async_read_some(ba::buffer(data),
                                       [&] (const boost::system::error_code &ec,
                                            const size_t length)
                {
                    if (ec) {
                        throw boost::system::system_error(ec);
                    }

                    const auto msg = Clientmessage::fromString(
                            std::string(data.begin(), data.begin()+length));

                    // Received valid Clientmessage
                    // Log it!
                    const auto& doormessage = msg.doormessage();
                    _l("Received message", LogLevel::info);
                    _l((std::string)"  token: " + msg.web_address(),
                       LogLevel::info);
                    _l((std::string)"  open: " + std::to_string(msg.isOpen()),
                      LogLevel::info);
                    _l((std::string)"  button lock: " + std::to_string(doormessage.isLockButton),
                      LogLevel::info);
                    _l((std::string)"  button unlock: " + std::to_string(doormessage.isUnlockButton),
                      LogLevel::info);
                    _l((std::string)"  emergency open: " + std::to_string(doormessage.isEmergencyUnlock),
                      LogLevel::info);

                    // Emit the new message
                    emit new_clientmessage(msg);

                    // Wait for new message
                    receiveMessage();
                });
            };

            receiveMessage();
            _io_service.run();
        }
        catch(const Response &err) {
            _l(err.message, LogLevel::error);
        }
        catch(const std::exception &err) {
            _l(LogLevel::error, err.what());
        }

        sleep(1);
    } while(1);

    _io_service.reset();
}
