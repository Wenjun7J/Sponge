#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) { _rto = retx_timeout; }

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if (_receiver_ackno + (_window_size == 0 ? 1 : _window_size) <= _next_seqno) {
    } else {
        while (_remaining_size > 0 && !_closed) {
            TCPSegment segment;
            if (_next_seqno == 0) {
                segment.header().syn = true;
            }
            segment.header().seqno = wrap(_next_seqno, _isn);
            if (segment.header().syn) _remaining_size--;
            segment.payload() = Buffer(_stream.read(min(TCPConfig::MAX_PAYLOAD_SIZE, _remaining_size)));
            _remaining_size -= segment.payload().size();
            if (stream_in().buffer_empty() && stream_in().input_ended() && _remaining_size > 0) {
                segment.header().fin = true;
                _closed = true;
                _remaining_size --;
            }
            if (segment.payload().size() + segment.header().syn + segment.header().fin > 0) {
                _segments_out.push(segment);
                _outstanding_segment.push(make_pair(_next_seqno , segment));
                _bytes_in_flight += segment.payload().size() + segment.header().syn + segment.header().fin;
                _next_seqno = stream_in().bytes_read() + 1/*syn*/ + segment.header().fin; 
                if (_timer_stop) {
                    _timer_stop = false;
                    _elapsed_time = 0;
                }
            } else {
                break;
            }
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size = window_size;
    _remaining_size = window_size == 0 ? 1 : window_size;
    uint64_t absolute_ackno = unwrap(ackno, _isn, stream_in().bytes_read());
    if (absolute_ackno > _next_seqno) {
        //wrong ack, do nothing
    } else {
        if (absolute_ackno > _receiver_ackno) {
            _receiver_ackno = absolute_ackno;
            //_next_seqno = absolute_ackno;
            _rto = _initial_retransmission_timeout;
            _elapsed_time = 0;
            _retry_cnt = 0;
        }
        while (!_outstanding_segment.empty()) {
            auto &it = _outstanding_segment.top();
            if (it.first + it.second.payload().size() + it.second.header().syn + it.second.header().fin <= absolute_ackno) {
                _bytes_in_flight -= (it.second.payload().size() + it.second.header().syn + it.second.header().fin);
                _outstanding_segment.pop();
            } else {
                break;
            }
        }

        if (_outstanding_segment.empty()) {
            _timer_stop = true; //means stop;
            _elapsed_time = 0;
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _tick_time += ms_since_last_tick; 
    if (!_timer_stop) {
        _elapsed_time += ms_since_last_tick;
        if (_elapsed_time >= _rto) {
            auto &it = _outstanding_segment.top();
            _segments_out.push(it.second);
            if (_window_size != 0) {
                _rto = 2 * _rto;
                _retry_cnt ++;
            }
            _elapsed_time = 0;
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retry_cnt; }

void TCPSender::send_empty_segment() {
    TCPSegment tcp_segment;
    tcp_segment.header().seqno = wrap(stream_in().bytes_read(), _isn);
    _segments_out.push(tcp_segment);
}
