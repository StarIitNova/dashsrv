#include <Minecraft/String.h>

#include <unordered_map>

#define ESC "\033"

namespace Minecraft {

    std::string EscapeToAnsi(const std::string &input) {
        std::string out;

        static const std::unordered_map<char, std::string> colorMap = {
            {'0', ESC "[30m"}, // Black
            {'1', ESC "[34m"}, // Dark Blue
            {'2', ESC "[32m"}, // Dark Green
            {'3', ESC "[36m"}, // Dark Aqua / Cyan
            {'4', ESC "[31m"}, // Dark Red
            {'5', ESC "[35m"}, // Dark Purple / Magenta
            {'6', ESC "[33m"}, // Gold / Yellow
            {'7', ESC "[37m"}, // Gray
            {'8', ESC "[90m"}, // Dark Gray
            {'9', ESC "[94m"}, // Blue
            {'a', ESC "[92m"}, // Green
            {'b', ESC "[96m"}, // Aqua
            {'c', ESC "[91m"}, // Red
            {'d', ESC "[95m"}, // Light Purple
            {'e', ESC "[93m"}, // Yellow
            {'f', ESC "[97m"}, // White
            {'r', ESC "[0m"}   // Reset
        };

        static const std::unordered_map<char, std::string> styleMap = {
            {'l', ESC "[1m"}, // Bold
            {'n', ESC "[4m"}, // Underline
            {'o', ESC "[3m"}, // Italic
            {'m', ESC "[9m"}, // Strikethrough
            {'k', ""}         // Obfuscated -> ignore (optional)
        };

        std::string_view section = "\xC2\xA7";
        for (size_t i = 0; i < input.size(); ++i) {
            if (i + 2 < input.size() && input.substr(i, 2) == section) {
                char code = input[i + 2];
                i += 2;
                if (colorMap.count(code)) {
                    out += colorMap.at(code);
                } else if (styleMap.count(code)) {
                    out += styleMap.at(code);
                }
            } else {
                out += input[i];
            }
        }

        out += ESC "[0m";

        return out;
    }

}
