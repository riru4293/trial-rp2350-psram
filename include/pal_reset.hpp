#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace pal
{
    /**
     * @brief システムパニック時の緊急停止・リセット処理を実行します。
     * 
     * この関数は、システム異常が発生した時に呼び出されます。
     * ウォッチドグを起動してリセットをトリガーしながら、
     * エラーメッセージをUART経由で送信します。
     * 
     * @param msg パニックメッセージ。リセット後に getDyingMessage() で取得可能です。
     * 
     * @details
     * - 関数は戻りません（noreturn）
     * - 複数コアからの呼び出しは最初のコアのみ処理、他コアは無限ループで停止
     * - 割り込みを無効化してリセットまでの確実性を確保
     * - ウォッチドグはUART送信開始前に起動
     * - UART送信はベストエフォート（タイムアウト時は途中で打ち切り）
     * 
     * @note リセット後、メッセージは getDyingMessage() で回収必要
     */
    [[noreturn]] void panic(std::string const &msg);

    /**
     * @brief 直前のパニックメッセージを取得します。
     * 
     * panic() が呼び出されて以降、リセットを経由してブートした場合、
     * 呼び出し直後にこの関数を実行することでパニックメッセージを取得できます。
     * 
     * @return パニックメッセージが存在する場合は文字列ビューを返します。
     *         メッセージが無い場合は std::nullopt を返します。
     * 
     * @details
     * - この関数は呼び出すたびにメッセージをクリアします
     * - メッセージは最大99文字（ヌル終端含む）
     * - 同じメッセージを複数回取得することはできません
     */
    std::optional<std::string_view> getDyingMessage(void);
}
