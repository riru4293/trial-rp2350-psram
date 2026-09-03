#pragma once

/**
 * @file pal_reset_struct.hpp
 * @brief Data types describing a software reset.
 *
 * @details
 * Defines the reset cause and diagnostic message for the reset that preceded
 * the current startup. @ref ResetContext exposes its message as a
 * `std::string_view` into reset-state storage; consume or copy it before
 * performing another reset.
 */

/* C++ standard library */
#include <string_view>
#include <cstdint>

namespace pal
{
    /**
     * @brief Identifies the software-reset entry point that initiated a reset.
     */
    enum class ResetKind : std::uint8_t
    {
        Panic,      //!< Reset requested for an unrecoverable failure.
        SoftReset,  //!< Ordinary software reset.
    };

    /**
     * @brief Reset cause and diagnostic message from the preceding startup.
     */
    struct ResetContext
    {
        /**
         * @brief Construct a reset context.
         *
         * @param kind [in] Reset cause.
         * @param msg [in] Diagnostic message.
         */
        explicit ResetContext(ResetKind kind, std::string_view msg
                ) noexcept : kind(kind), msg(msg) {}

        ResetKind const kind;       //!< Reset cause.
        std::string_view const msg; //!< Diagnostic message.
    };
}
