#pragma once

// local includes
#include "k273/logging.h"
#include "k273/exception.h"

// std includes
#include <string>
#include <vector>

namespace K273 {
    namespace Runner {

        struct Config {
            Config(int argc, char** argv) :
                log_filename("logfile.log") {

                // create args as string
                char** ptstr = argv;
                for (int ii=0; ii<argc; ii++, ptstr++) {
                    if (*ptstr == nullptr) {
                        break;
                    }

                    this->args.emplace_back(*ptstr);
                }
            }

            std::vector <std::string> args;
            std::string log_filename;
        };

        template <typename F> int Main(F f, Config& config) {

            K273::loggerSetup(config.log_filename, K273::Logger::LOG_DEBUG);

            try {
                f(config.args);

            } catch (const K273::Assertion &exc) {
                fprintf(stderr, "Assertion : %s\n", exc.getMessage().c_str());
                fprintf(stderr, "Stacktrace :\n%s\n", exc.getStacktrace().c_str());
                return 1;
            } catch (const K273::SysException &exc) {
                fprintf(stderr, "SysException : %s\n", exc.getMessage().c_str());
                fprintf(stderr, "  Get error code = %d\n", exc.getErrorCode());
                fprintf(stderr, "  Get error string = %s\n", exc.getErrorString().c_str());
                fprintf(stderr, "Stacktrace :\n%s\n", exc.getStacktrace().c_str());
                return 1;

            } catch (const K273::Exception &exc) {
                fprintf(stderr, "Assertion : %s\n", exc.getMessage().c_str());
                fprintf(stderr, "Stacktrace :\n%s\n", exc.getStacktrace().c_str());
                return 1;

            } catch (...) {
                fprintf(stderr, "Unknown exception caught\n");
                return 1;
            }

            return 0;
        }

    }
}
