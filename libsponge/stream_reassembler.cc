#include "stream_reassembler.hh"
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

#define WINDOW_BEGIN (_output.bytes_written())
#define WINDOW_END (WINDOW_BEGIN + _capacity - _output.buffer_size())

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _buffer(), _eof(false) {}

void StreamReassembler::assemble() {
    auto it = _buffer.begin();
    while (it != _buffer.end()) {
        size_t begin = it->first;
        string data = it->second;
        if (begin == WINDOW_BEGIN) {
            _buffer.erase(it++);
            size_t written = _output.write(data);
            begin += written;
            data.erase(0, written);
            if (!data.empty()) {
                _buffer.insert(make_pair(begin, data));
                break;
            }
        } else {
            break;
        }
    }
}

void StreamReassembler::put_data_into_buffer(std::string data, size_t index) {
    if ((index + data.length() <= WINDOW_BEGIN) || (index >= WINDOW_END)) {
        //unacceptable
    } else {
        if (index < WINDOW_BEGIN) {
            data.erase(0, WINDOW_BEGIN - index);
            index = WINDOW_BEGIN;
        }
        for (auto it = _buffer.begin(); it != _buffer.end() && !data.empty(); it++) {
            if (it->first > index) {
                size_t len = it->first - index;
                _buffer.insert(make_pair(index, data.substr(0, len)));
                data.erase(0, len);
                index += len;
            } else if (it->first < index) {
                if (it->first + it->second.length() <= index) {
                    //no cross
                } else {
                    size_t cross_len = it->first + it->second.length() - index;
                    data.erase(0, cross_len);
                    if (!data.empty()) {
                        index += cross_len;
                    }
                }
            }

            if (index == it->first) {
                data.erase(0, it->second.length());
                if (!data.empty()) {
                    index += it->second.length();
                }
            }
        }
        if (!data.empty()) {
            if (index >= WINDOW_END) {
                //unacceptable
            } else {
                size_t len = WINDOW_END - index;
                _buffer.insert(make_pair(index, data.substr(0, len)));
            }

        }
    }
}


//! \details This function accepts a substring (aka a segment) of bytes
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t data_end = data.length() + index;
    if (eof && data_end <= WINDOW_END) {
        _eof = true;
    }

    put_data_into_buffer(data, index);
    assemble();

    if (_eof && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    size_t total = 0;
    for (auto it : _buffer) total += it.second.length();
    return total;
}

bool StreamReassembler::empty() const { return _buffer.empty(); }
