#pragma once
#include <kibble/logger/logger_sink.h>
#include <kibble/net/tcp_connector.h>
#include <kibble/net/tcp_stream.h>

namespace erwin
{

class NetSink: public kb::klog::Sink
{
public:
    virtual ~NetSink();
    virtual void send(const kb::klog::LogStatement& stmt, const kb::klog::LogChannel& chan) override;
    virtual void send_raw(const std::string& message) override;
    virtual void on_attach() override;
    bool connect(const std::string& server, uint16_t port);

private:
	std::string server_;
	kb::net::TCPStream* stream_ = nullptr;
};

} // namespace erwin