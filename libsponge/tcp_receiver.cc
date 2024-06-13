#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn) {
        _syn = true;
        _isn = seg.header().seqno.raw_value();
    } 
    if (seg.header().fin) {
        _last_byte = unwrap(seg.header().seqno, WrappingInt32(_isn), stream_out().bytes_written());
    } 
    
    uint64_t index = unwrap(seg.header().seqno, WrappingInt32(_isn), stream_out().bytes_written());
    bool eof = false;
    if (index + stream_out().bytes_written() + 1 >= _last_byte) {
        eof = true;
    }
    std::string data(seg.payload().str());
    if (index + seg.header().syn > 0) {
        _reassembler.push_substring(data, index - 1 + seg.header().syn, eof);
    } else {
        //unacceptable
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn) {
        return std::nullopt;
    }
    uint64_t ackno = stream_out().bytes_written() + 1;
    if (stream_out().input_ended()) {
        ackno += 1;
    }
    return wrap(ackno, WrappingInt32(_isn));
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
