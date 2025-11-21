#include "log.hpp"
#include <spdlog/sinks/hourly_file_sink.h>
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

namespace orion::common
{
std::shared_ptr<spdlog::logger> init_hourly_logger(const std::string& name)
{
static std::mutex mtx;
std::scoped_lock lock(mtx);

if (auto existing = spdlog::get(name)) {
    return existing;
}

fs::path logdir = fs::path("dat") / "log"; // repository-relative
std::error_code error_code;
fs::create_directories(logdir, error_code);

// hourly_file_sink_mt automatically appends timestamp, just provide base name
auto base_filename = logdir / (name + ".log");
auto file_sink = std::make_shared<spdlog::sinks::hourly_file_sink_mt>(base_filename.string(), false);

auto logger = std::make_shared<spdlog::logger>(name, file_sink);
logger->set_level(spdlog::level::info);
logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

spdlog::register_logger(logger);
return logger;
}
}