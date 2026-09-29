// The MC4 conversation with one plugin process: the handshake, requests, shutdown.
module mcxx.plugin.host;

import std;
import nlohmann.json;
import mcxx.base;
import mcxx.os;
import mcxx.msa;
import mcxx.plugin;
import mcxx.plugin.wire;
import mcxx.features;

namespace mcxx::plugin::host {

using wire::Json;

Session::Session(std::string name, std::vector<std::string> command, std::string directory, std::chrono::milliseconds timeout)
    : name_ { std::move(name) }, command_ { std::move(command) }, directory_ { std::move(directory) }, timeout_ { timeout } {}

std::string Session::fail(std::string why) {
    if (process_) process_->kill();
    process_.reset();
    dead_ = why;
    base::trace::info("plugins", "plugin {} failed: {}", name_, why);
    base::trace::count("plugins.failed");
    return why;
}

std::expected<Json, std::string> Session::exchange(const Json& message, std::int64_t id) {
    if (!process_) return std::unexpected(dead_.empty() ? std::string { "it is not running" } : dead_);
    if (!process_->write(wire::line(message))) {
        // It stopped reading: most likely it is gone, and the read says how.
        auto gone = process_->read_line(std::chrono::steady_clock::now() + std::chrono::milliseconds { 100 });
        return std::unexpected(fail(gone ? std::string { "it does not read its input" } : gone.error()));
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout_;
    for (;;) {
        auto text = process_->read_line(deadline);
        if (!text) return std::unexpected(fail(text.error() == "it did not answer in time"
                                                   ? std::format("it did not answer in {} ms", timeout_.count())
                                                   : text.error()));
        auto answer = wire::message_from(*text);
        if (!answer) return std::unexpected(fail(std::format("it wrote what is not an MC4 message: {}", answer.error())));
        if (!answer->contains("id") || (*answer)["id"] != id) {
            if ((*answer)["type"] == "error") return std::unexpected(fail(std::format("it said: {}", answer->value("message", std::string {}))));
            return std::unexpected(fail("it answered a request it was not asked"));
        }
        return std::move(*answer);
    }
}

std::expected<std::vector<wire::ProviderInfo>, std::string> Session::start() {
    std::lock_guard lock { mutex_ };
    base::trace::Span span { "plugins", "start", name_ };
    auto process = Process::start(command_, directory_, base::trace::enabled("plugins", base::trace::Level::debug));
    if (!process) return std::unexpected(fail(process.error()));
    process_.emplace(std::move(*process));
    Json hello = Json::object();
    hello["type"] = "hello";
    hello["id"] = 0;
    hello["host"] = "mcxx 0.1.0";
    hello["protocols"] = Json::array({ wire::PROTOCOL });
    auto answer = exchange(hello, 0);
    if (!answer) return std::unexpected(answer.error());
    if ((*answer)["type"] == "error")
        return std::unexpected(fail(std::format("it speaks none of the protocols offered ({})", answer->value("message", std::string {}))));
    if ((*answer)["type"] != "welcome" || !answer->contains("protocol") || (*answer)["protocol"] != wire::PROTOCOL || !answer->contains("providers") ||
        !(*answer)["providers"].is_array())
        return std::unexpected(fail("its answer to hello is not a welcome with protocol 1 and its providers (MC4-6.2-1)"));
    std::vector<wire::ProviderInfo> providers;
    for (const auto& p : (*answer)["providers"]) {
        auto info = wire::provider_from(p);
        if (!info) return std::unexpected(fail(std::format("it describes a provider wrongly: {}", info.error())));
        providers.push_back(std::move(*info));
    }
    return providers;
}

std::expected<Json, std::string> Session::request(Json message) {
    std::lock_guard lock { mutex_ };
    const std::int64_t id { next_id_++ };
    message["id"] = id;
    auto answer = exchange(message, id);
    if (!answer) return answer;
    if ((*answer)["type"] == "error") return std::unexpected(std::format("it could not: {}", answer->value("message", std::string {})));
    return answer;
}

void Session::shutdown() {
    std::lock_guard lock { mutex_ };
    if (!process_) return;
    Json bye = Json::object();
    bye["type"] = "shutdown";
    bye["id"] = next_id_++;
    process_->write(wire::line(bye));
    // A moment to exit on its own, then it is killed (Process::kill).
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds { 200 };
    (void)process_->read_line(until);
    process_.reset();
}

} // namespace mcxx::plugin::host
