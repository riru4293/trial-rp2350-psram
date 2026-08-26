#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace pal
{
    [[noreturn]] void panic(std::string const &msg);
    std::optional<std::string_view> getDyingMessage(void);
}
