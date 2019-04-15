﻿#include <thread>
#include <atomic>
#include <string>
#include <string_view>
#include <chrono>

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;

inline static const fs::path & get_file_path(const fs::path & arg) {
    return arg;
}

#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

inline static std::string get_file_path(const fs::path & arg) {
    return fs::canonical(arg).string();
}

#endif

#include <list>
#include <vector>

#include <iostream>
#include <fstream>

#include <regex>

inline static std::string_view trim_right(const std::string & arg) {
    if (arg.empty()) {
        return ""sv;
    }
    const auto varLastPos =
        arg.find_last_not_of(" \r\n"sv);
    if (varLastPos == std::string::npos) {/*empty line ...*/
        return ""sv;
    }
    return std::string_view{ arg.c_str(), 1 + varLastPos };
}

inline static void cast_a_file(const fs::path & arg, bool ignoreBom) {

    std::list<std::string > varLines;

    const auto & varFileName = get_file_path(arg);
    {
        std::ifstream varInput{ varFileName ,std::ios::binary };
        if (!varInput.is_open()) {
            return;
        }
        while (varInput.good()) {
            auto & varLine = varLines.emplace_back();
            std::getline(varInput, varLine);
        }
    }

    if (varLines.empty()) {
        return;
    }

    std::vector< std::string_view > varAllAboutToWrite;
    varAllAboutToWrite.reserve(varLines.size());
    for (const auto & varLine : varLines) {
        varAllAboutToWrite.push_back(trim_right(varLine));
    }

    while (!varAllAboutToWrite.empty()) {
        if (varAllAboutToWrite.back().empty()) {
            varAllAboutToWrite.pop_back();
        } else {
            break;
        }
    }

    const constexpr char varRawBom[4]{ (char)(0x000ef),
                (char)(0x000bb),
                (char)(0x000bf),
                (char)(0x00000) };

    std::ofstream varOutPut{ varFileName,std::ios::binary };
    if (varAllAboutToWrite.empty()) {
        if (ignoreBom) {
            varOutPut << "\n"sv;
        } else {
            const std::string_view varBom{ varRawBom,3 };
            varOutPut << varBom << "\n"sv;
        }

        return;
    }

    if (!ignoreBom) {
        const auto & varFirstLine = varAllAboutToWrite[0];
        bool varNeedWriteBom = false;
        if (varFirstLine.size() < 3) {
            varNeedWriteBom = true;
        } else {
            if ((varFirstLine[0] != varRawBom[0]) ||
                (varFirstLine[1] != varRawBom[1]) ||
                (varFirstLine[2] != varRawBom[2])) {
                varNeedWriteBom = true;
            }
        }
        if (varNeedWriteBom) {
            const std::string_view varBom{ varRawBom,3 };
            varOutPut << varBom;
        }
    }

    for (const auto & varLine : varAllAboutToWrite) {
        varOutPut << varLine << "\n"sv;
    }

}

inline static void castCRLFOrCRToLF(const fs::path & arg) {
    if (!fs::is_directory(arg)) {
        return;
    }
    const static std::wregex varCheckFormat{
        LR"!(^[^.].*\.(?:(?:txt)|(?:pri)|(?:pro)|(?:cpp)|(?:c)|(?:h)|(?:hpp)|(?:hxx)|(?:qml)|(?:tex))$)!" ,
        std::regex_constants::icase | std::regex_constants::ECMAScript
    };
    const static std::wregex varCheckIgnoreBOM{
        LR"!(^[^.].*\.(?:(?:pri)|(?:pro)|(?:txt))$)!" ,
        std::regex_constants::icase | std::regex_constants::ECMAScript
    };
    std::list< std::pair< fs::path, std::wstring > > varAboutToDo;
    {
        fs::recursive_directory_iterator varIt{ arg };
        fs::recursive_directory_iterator const varEnd;
        for (; varIt != varEnd; ++varIt) {
            if (!fs::is_regular_file(*varIt)) {
                continue;
            }
            const auto & varFileNamePath = varIt->path();
            auto varFileName =
                varFileNamePath.filename().wstring();
            if (std::regex_match(varFileName, varCheckFormat)) {
                varAboutToDo.emplace_back(varFileNamePath, std::move(varFileName));
            }
        }
    }
    for (const auto & varI : varAboutToDo) {
        const auto varIgnoreBOM =
            std::regex_match(varI.second, varCheckIgnoreBOM);
        cast_a_file(varI.first, varIgnoreBOM);
    }
}

int main(int, char **) {
    fs::path varPath{ CURRENT_DEBUG_PATH };
    fs::path varPaths[]{
        varPath / ".." / "chapter01",
        varPath / ".." / "chapter02",
        varPath / ".." / "latex_book",
        varPath / ".." / "qt_quick_book_private",
        varPath / ".." / "sstd_clean_code",
        varPath / ".." / "sstd_copy_qml",
        varPath / ".." / "sstd_library",
        varPath / ".." / "sstd_qt_qml_quick_library"
    };

    std::atomic< int > varThreadCount{ 0 };
    for (const auto & varI : varPaths) {
        ++varThreadCount;

        /*限制线程数量*/
        while (varThreadCount.load() > (std::thread::hardware_concurrency() + 1)) {
            std::this_thread::sleep_for(10ms);
        }

        std::thread([varPath = varI, &varThreadCount]() {
            try {
                castCRLFOrCRToLF(varPath);
            } catch (...) {
            }
            --varThreadCount;
        }).detach();

    }

    /*等待所有线程完成*/
    if (varThreadCount.load() > 0) {
        std::this_thread::sleep_for(10ms);
    }

}
