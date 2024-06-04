#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : capacity_(capacity), size_(0), data_(), 
                                                input_end_(false), total_written_(0), total_read_(0), eof_(false) {}

size_t ByteStream::write(const string &data) {
    size_t remaining = remaining_capacity();
    if (remaining > 0) {
        data_ += data.substr(0, remaining);
    }
    total_written_ += (remaining - remaining_capacity());
    return remaining - remaining_capacity();
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return data_.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    total_read_ += len > data_.length() ? data_.length() : len;
    data_.erase(0, len);
    if (data_.empty() && input_ended()) {
        eof_ = true;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ans = peek_output(len);
    pop_output(ans.length());
    return ans;
}

void ByteStream::end_input() { 
    input_end_ = true; 
    if (buffer_empty()) {
        eof_ = true; 
    }
}

bool ByteStream::input_ended() const { return input_end_; }

size_t ByteStream::buffer_size() const { return data_.length(); }

bool ByteStream::buffer_empty() const { return data_.length() == 0; }

bool ByteStream::eof() const { return eof_; }

size_t ByteStream::bytes_written() const { return total_written_; }

size_t ByteStream::bytes_read() const { return total_read_; }

size_t ByteStream::remaining_capacity() const { return capacity_ - data_.length(); }
