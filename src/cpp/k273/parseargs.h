#pragma once

// std includes
#include <vector>
#include <string>

namespace K273 {

    class PargeArgs {

      public:
        PargeArgs(const std::vector<std::string>& args);
        ~PargeArgs();

      public:
        int more();
        bool isNext(const std::string& next);

        const std::string& getString();
        int getInt();
        const char* getCString();

      private:
        const std::vector<std::string>& args;
        size_t count;
    };

}
