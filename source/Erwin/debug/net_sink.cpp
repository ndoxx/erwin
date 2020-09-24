#include "debug/net_sink.h"
#include "kibble/logger/logger_common.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iostream>

namespace erwin
{

/*static const std::regex ansi_regex("\033\\[.+?m"); // matches ANSI codes
static std::string strip_ansi(const std::string& str) { return std::regex_replace(str, ansi_regex, ""); }*/

static constexpr char s_base64_chars[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
    's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static std::string base64_encode(const std::string data)
{
    size_t in_len = data.size();
    size_t out_len = 4 * ((in_len + 2) / 3);
    std::string ret(out_len, '\0');
    size_t ii;
    char* p = const_cast<char*>(ret.c_str());

    for(ii = 0; ii < in_len - 2; ii += 3)
    {
        *p++ = s_base64_chars[(data[ii] >> 2) & 0x3F];
        *p++ = s_base64_chars[((data[ii] & 0x3) << 4) | (static_cast<int>(data[ii + 1] & 0xF0) >> 4)];
        *p++ = s_base64_chars[((data[ii + 1] & 0xF) << 2) | (static_cast<int>(data[ii + 2] & 0xC0) >> 6)];
        *p++ = s_base64_chars[data[ii + 2] & 0x3F];
    }
    if(ii < in_len)
    {
        *p++ = s_base64_chars[(data[ii] >> 2) & 0x3F];
        if(ii == (in_len - 1))
        {
            *p++ = s_base64_chars[((data[ii] & 0x3) << 4)];
            *p++ = '=';
        }
        else
        {
            *p++ = s_base64_chars[((data[ii] & 0x3) << 4) | (static_cast<int>(data[ii + 1] & 0xF0) >> 4)];
            *p++ = s_base64_chars[((data[ii + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    return ret;
}

NetSink::~NetSink()
{
    if(stream_)
    {
    	// Notify server before closing connection
    	stream_->send("{\"action\":\"disconnect\"}");
        delete stream_;
    }
}

bool NetSink::connect(const std::string& server, uint16_t port)
{
    server_ = server;
    stream_ = kb::net::TCPConnector::connect(server, port);
    return (stream_ != nullptr);
}

void NetSink::on_attach()
{
	if(stream_ != nullptr)
	{
	    std::stringstream ss;
		// Notify new connection
		ss << "{\"action\":\"connect\", "
		   << "\"peer_ip\":\""   << stream_->get_peer_ip() << "\", "
		   << "\"peer_port\":\"" << uint32_t(stream_->get_peer_port()) << "\"}";
    	stream_->send(ss.str());

	    // Send subscribed channels to server
	    ss.str("");
	    ss << "{\"action\":\"set_channels\", \"channels\":["; 
	    for(size_t ii=0; ii<subscriptions_.size(); ++ii)
	    {
	    	const auto& desc = subscriptions_[ii];
	    	ss << '\"' << desc.name << '\"';
	    	if(ii<subscriptions_.size()-1)
	    		ss << ',';
	    }
	    ss << "]}";
    	stream_->send(ss.str());
	}
}

void NetSink::send(const kb::klog::LogStatement& stmt, const kb::klog::LogChannel& chan)
{
    // Send JSON formatted message
    std::stringstream ss;
    ss << "{\"action\":\"msg\", \"channel\":\"" << chan.name 
       << "\", \"type\":\"" << uint32_t(stmt.msg_type)
       << "\", \"severity\":\"" << uint32_t(stmt.severity) 
       << "\", \"timestamp\":\"" << std::chrono::duration_cast<std::chrono::duration<float>>(stmt.timestamp).count() 
       << "\", \"line\":\"" << stmt.code_line 
       << "\", \"file\":\"" << stmt.code_file 
       << "\", \"message\":\"" << base64_encode(stmt.message + "\033[0m") << "\"}";
    stream_->send(ss.str());
}

void NetSink::send_raw(const std::string& message) { stream_->send(message); }

} // namespace erwin