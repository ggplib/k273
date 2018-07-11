// local includes
#include "common.h"

// k273 includes
#include <k273/util.h>
#include <k273/exception.h>
#include <k273/parseargs.h>
#include <kelvin/sharedmem.h>

// std includes
#include <vector>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;

///////////////////////////////////////////////////////////////////////////////

// how much to keep in echo queue
static const uint64_t queue_space = 128;

///////////////////////////////////////////////////////////////////////////////

void go(vector <string>& args) {

    // setup signals
    setupSignals();

    // set up queus
    EchoProducer echo(EchoQueueSize);
    EchoConsumer trimmer(EchoQueueSize);
    RequestConsumer request(RequestQueueSize);

    // setup shared memory
    Kelvin::SharedMemory* request_memory = Kelvin::SharedMemory::create(request_name, request.getMemorySize());
    Kelvin::SharedMemory* echo_memory = Kelvin::SharedMemory::create(echo_name, echo.getMemorySize());

    // for cleanup
    std::unique_ptr <Kelvin::SharedMemory> _echo_memory(echo_memory);
    std::unique_ptr <Kelvin::SharedMemory> _request_memory(request_memory);

    // set memory / init queues
    request.setMemory(request_memory->accessMemory(), true);

    echo.setMemory(echo_memory->accessMemory(), true);
    trimmer.setMemory(echo_memory->accessMemory());

    l_debug("waiting for requests");

    uint64_t rxd_count = 0;
    uint64_t txd_count = 0;
    uint64_t empty_count = 0;

    // clients must be sequenced
    uint32_t client_seqs[256];
    memset(&client_seqs, 0, sizeof(client_seqs));

    while (run_flag) {

        Event* event = (Event*) request.next();
        if (event != nullptr) {

            // lock at this point
            uint32_t client_id = event->client_id;
            uint64_t seq = event->seq;

            // unlock
            request.consume();

            rxd_count++;

            if (client_id > 255) {
                l_warning("invalid client id %d", client_id);

            } else {
                // check seq
                uint64_t expect = client_seqs[client_id];

                if (expect != seq) {
                    if (seq == 0) {
                        l_info("client_id %d: reset", client_id);
                        client_seqs[client_id] = 0;
                    } else {
                        l_warning("client_id %u: %lu %lu", client_id, expect, seq);
                    }
                }

                client_seqs[client_id] = seq + 1;

                // jolly good, now lets send it back to client
                uint8_t* mem = echo.reserveBytes(sizeof(Event));
                // ZZZ this is wrong.  We have 1-n access to write, so why loop.  The reality is we
                // might be full for some reason and should guard against that.

                //while (mem == nullptr) {
                //    cpuRelax();
                //    mem = echo.reserveBytes(sizeof(Event));
                //}
                ASSERT (mem != nullptr);

                Event* out = (Event*) mem;

                // at this point we have a lock
                out->ticks = rdtsc();
                out->client_id = client_id;
                out->seq = seq;

                echo.publish();
            }

        } else {
            cpuRelax();

            if (txd_count + queue_space < rxd_count && trimmer.next(true) != nullptr) {
                txd_count++;
            }

            empty_count++;
        }
    }

    l_debug("final stats - rx'd: %lu, empty: %lu ", rxd_count, empty_count);
}

///////////////////////////////////////////////////////////////////////////////

#include <k273/runner.h>

int main(int argc, char** argv) {
    K273::Runner::Config config(argc, argv);
    config.log_filename = "server.log";

    return K273::Runner::Main(go, config);
}

