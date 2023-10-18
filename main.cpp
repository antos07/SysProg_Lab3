#include <algorithm>
#include <iostream>
#include <iterator>
#include <format>
#include <fstream>
#include <map>
#include <regex>
#include <string>
#include <vector>

struct HighlightedSegment {
    std::size_t startPos;
    std::size_t endPos;
    std::string_view color;
};


int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << std::format("Usage: {} <filename>", argv[0]) << std::endl;
        return EXIT_FAILURE;
    }

    // Read file.
    std::ifstream inputFile{argv[1]};
    if (!inputFile.is_open()) {
        std::perror(argv[0]);
        return EXIT_FAILURE;
    }
    std::string sourceCode{std::istreambuf_iterator<char>{inputFile}, std::istreambuf_iterator<char>{}};

    std::string commentRegex{R"(//.*|/\*(?:.|\n)*?\*/)"};
    std::string stringRegex{R"(`(?:.|\n)*?`|"(?:\\"|.)*?"|'\\?.')"};
    std::string keywordRegex{"\\b(?:break|default|func|interface|select|case|defer|go|map|struct|chan|else|goto"
                             "|package|switch|const|fallthrough|if|range|type|continue|for|import|return|var)\\b"};
    std::string operatorRegex{R"(<-|[+\-|&]{2}|(?:>>|<<|&\^|[+\-*/%&|^=<>!])=?|~|:=)"};
    std::string delimiterRegex{R"(\.{3}|[()\[\]{},;.:])"};
    std::string identifierRegex{R"(\b[_[:alpha:]]\w*\b)"};
    std::string numberRegex{
            // Decimal float
            R"(\d(?:_?\d)*\.(?:\d(?:_?\d)*)?(?:[eE][+-]?\d(?:_?\d)*)?)"
            R"(|\d(?:_?\d)*[eE][+-]?\d(?:_?\d)*)"
            R"(|\.\d(?:_?\d)*(?:[eE][+-]?\d(?:_?\d)*)?)"
            // Word boundary check for next statements (no leading or trailing '.' is possible)
            R"(|\b(?:)"
            // Hex float
            R"(0[xX](?:_?[0-9a-fA-F])*(?:\.(?:_?[0-9a-fA-F])*)?[pP][+-]?\d+)"
            // Decimal integer
            R"(|[1-9](?:_?\d)*)"
            // Hex integer
            R"(|0[xX](?:_?[0-9a-fA-F])+)"
            // Octal integer
            R"(|0[oO]?(?:_?[0-7])+)"
            // Just 0
            "|0"
            // Word boundary checks
            R"()\b)"
    };
    std::string errorRegex{R"(\S+)"};

    std::regex fullRegex{std::format("({})|({})|({})|({})|({})|({})|({})|({})", commentRegex, stringRegex,
                                     keywordRegex, operatorRegex, identifierRegex, numberRegex,
                                     delimiterRegex, errorRegex)};

    std::regex_iterator regexIterator{sourceCode.begin(), sourceCode.end(), fullRegex};
    const std::regex_iterator<std::string::iterator> emptyRegexIterator{};
    const std::string_view lexemeClasses[]{"comment", "string", "keyword", "operator", "identifier", "number",
                                           "delimiter", "error"};
    const std::string_view colorCodes[]{
        "\033[36m", // Cyan for comments
        "\033[32m", // Green for strings
        "\033[33m", // Yellow for keywords
        "\033[34m", // Blue for operators
        "\033[35m", // Magenta for identifiers
        "\033[37m", // White for numbers
        "\033[31m", // Red for delimiters
        "\033[41m", // Red background for errors
    };
    std::vector<HighlightedSegment> highlightedSegments{};
    while (regexIterator != emptyRegexIterator) {
        for (int i{1}; i < regexIterator->size(); ++i) {
            if (!(*regexIterator)[i].matched) {
                continue;
            }
            HighlightedSegment segment{};
            segment.startPos = regexIterator->position();
            segment.endPos = segment.startPos + regexIterator->length() - 1;
            segment.color = colorCodes[i - 1];
            highlightedSegments.emplace_back(segment);
            break;
        }
        ++regexIterator;
    }

    auto segmentIterator{highlightedSegments.cbegin()};
    std::string_view resetColor{"\033[0m"};
    for (std::size_t i{0}; i < sourceCode.length(); ++i) {
        if (segmentIterator == highlightedSegments.cend() || i < segmentIterator->startPos) {
            std::cout << sourceCode[i];
            continue;
        }
        if (i == segmentIterator->startPos) {
            std::cout << segmentIterator->color;
        }
        std::cout << sourceCode[i];
        if (i == segmentIterator->endPos) {
            std::cout << resetColor;
            ++segmentIterator;
        }
    }

    return EXIT_SUCCESS;
}
