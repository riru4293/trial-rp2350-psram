#pragma once

/**
 * @file pal_reset.hpp
 * @brief Software-reset interface with reset-context handoff.
 *
 * @details
 * Declares reset entry points and the accessor for their associated
 * @ref ResetContext. Both reset entry points store the
 * supplied diagnostic message so it can be inspected after the next startup.
 * The stored context is consumable: @ref getResetContext returns it at most
 * once and returns an empty optional otherwise.
 *
 * Call getResetContext during single-core startup, before any core can request
 * a software reset. It does not synchronize with a concurrent reset request.
 *
 * @par Reset modes
 * @ref reset requests an ordinary software reset. @ref panic is intended for
 * unrecoverable failures: it disables interrupts on the calling core, waits
 * for the reset while repeatedly attempting to emit the diagnostic message on
 * UART0, and does not return. The reset occurs after approximately two
 * seconds; any subsequent calls made before then do not initiate another
 * reset and instead loop indefinitely.
 *
 * @see pal_reset_struct.hpp for the reset-context data types.
 */

#include "./pal_reset_struct.hpp"

/* C++ standard library */
#include <optional>
#include <string>

namespace pal
{
    /**
     * @brief Request an unrecoverable-failure reset.
     *
     * @param msg [in] Diagnostic message preserved in the reset context.
     */
    [[noreturn]] void panic(std::string const &msg) noexcept;

    /**
     * @brief Request an ordinary software reset.
     *
     * @param msg [in] Diagnostic message preserved in the reset context.
     */
    [[noreturn]] void reset(std::string const &msg) noexcept;

    /**
     * @brief Consume the reset context saved by a software reset.
     *
     * @return The saved context, or an empty optional when none is available.
     */
    std::optional<pal::ResetContext> getResetContext(void) noexcept;
}
