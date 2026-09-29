// mcxx.os: the process's standard error, through openkal's platform stream (the one place libmc++
// names openkal: A0.2.2).
module mcxx.os;

import std;
import openkal.stream;

namespace mcxx::os {

void write_standard_error(std::string_view text) {
    std::size_t done { 0 };
    while (done < text.size()) {
        const auto written = kal_stream_write(kal_stderr(), text.data() + done, text.size() - done);
        if (written <= 0) break;
        done += static_cast<std::size_t>(written);
    }
    kal_stream_flush(kal_stderr());
}

} // namespace mcxx::os
