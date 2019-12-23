// local includes
#include "common.h"

// k273 includes
#include <k273/util.h>
#include <k273/strutils.h>
#include <k273/exception.h>
#include <k273/parseargs.h>
#include <kelvin/sharedmem.h>

// std includes
#include <vector>
#include <unistd.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;

///////////////////////////////////////////////////////////////////////////////
// there is a lot of contention when this zero, reality is that noone will be updating this at same time
const int wait_time_usecs = 50;

///////////////////////////////////////////////////////////////////////////////

int getWaitTime(int min_wait_time_usecs) {
    if (min_wait_time_usecs == 0) {
        return 0;
    }

    srandom((unsigned int) rdtsc());
    while (1) {
        int x = random() % 1000;
        if (x > min_wait_time_usecs) {
            return x;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void go(vector <string>& args) {

    // setup signals
    setupSignals();

    // do args
    PargeArgs p(args);
    unsigned int client_id = p.getInt();

    // set up queus
    EchoConsumer echo(EchoQueueSize);
    RequestProducer request(RequestQueueSize);

    // setup shared memory
    Kelvin::SharedMemory* echo_memory = Kelvin::SharedMemory::attach(echo_name, echo.getMemorySize());
    Kelvin::SharedMemory* request_memory = Kelvin::SharedMemory::attach(request_name, request.getMemorySize());

    // for cleanup
    std::unique_ptr <Kelvin::SharedMemory> _echo_memory(echo_memory);
    std::unique_ptr <Kelvin::SharedMemory> _request_memory(request_memory);

    // setup shared memory
    echo.setMemory(echo_memory->accessMemory());
    request.setMemory(request_memory->accessMemory());

    // counts
    uint64_t ping_pong_count = 0;
    uint64_t request_full_count = 0;

    // stats
    uint64_t gmin = ~0, gave = 0, gmax = 0;
    uint64_t gmin_ping = ~0, gave_ping = 0, gmax_ping = 0;

    l_debug("publishing to %s", request_name);

    uint64_t report_time = get_time() + 5;

    while (run_flag) {
        // start of ping
        uint8_t* mem = request.reserveBytes(sizeof(Event));
        if (mem != nullptr) {
            Event* out = (Event*) mem;

            // at this point we have a lock
            uint64_t sent_at = out->ticks = rdtsc();
            out->client_id = client_id;
            out->seq = ping_pong_count;

            request.publish();

            // wait for pong on echo
            while (run_flag) {
                Event* in = (Event*) echo.next();

                if (in == nullptr) {
                    cpuRelax();
                    continue;
                }

                if (in->client_id != client_id) {
                    continue;
                }

                ASSERT_MSG(in->seq == ping_pong_count, fmtString("seqs wrong: %d %d", in->seq, ping_pong_count));

                // lock at this point
                uint64_t ticks = in->ticks;

                // do stats

                uint64_t t = rdtsc();
                uint64_t lag = t - ticks; //KelvinLib::toNanosecs(ticks, t);
                uint64_t ping_lag = t - sent_at; //KelvinLib::toNanosecs(sent_at, t);

                // 10 msecs - ignore
                if (lag < (1000 * 1000)) {
                    if (lag < gmin) {
                        gmin = lag;
                    }

                    if (lag > gmax) {
                        gmax = lag;
                    }

                    gave = (gave * ping_pong_count + lag) / (ping_pong_count + 1);

                } else {
                    l_debug("long time for reply %f usecs", lag / 1000.0);
                }

                // 10 mecs - ignore
                if (ping_lag < (1000 * 1000)) {
                    if (ping_lag < gmin_ping) {
                        gmin_ping = ping_lag;
                    }

                    if (ping_lag > gmax_ping) {
                        gmax_ping = ping_lag;
                    }

                    gave_ping = (gave_ping * ping_pong_count + ping_lag) / (ping_pong_count + 1);

                } else {
                    l_debug("long time for ping %f usecs", ping_lag / 1000.0);
                }

                ping_pong_count++;
                break;
            }

        } else {
            request_full_count++;
            cpuRelax();
            continue;
        }

        // randomized wait - will wait for some period
        int wait_usecs = getWaitTime(wait_time_usecs);
        double start_time_usecs = get_time() * 1000000;

        while (run_flag) {

            double cur_time_usecs = get_time() * 1000000;
            int elapsed_time = cur_time_usecs - start_time_usecs;
            if (elapsed_time > wait_usecs) {
                break;
            }

            // purge queue for a bit
            for (int ii=0; ii<1000; ii++) {
                Event* dummy = (Event*) echo.next();
                if (dummy == nullptr) {
                    cpuRelax();
                }
            }
        }

        double cur_time = get_time();
        if (cur_time > report_time) {
            l_debug("final stats : pings: %lu, request full %lu", ping_pong_count, request_full_count);
            l_debug("latency pongs (min/avg/max) %.2f/%.2f/%.2f", gmin_ping/1000.0, gave_ping/1000.0, gmax_ping/1000.0);
            l_debug("latency from server (min/avg/max) %.2f/%.2f/%.2f", gmin/1000.0, gave/1000.0, gmax/1000.0);
            report_time = cur_time + 5;

            gmin = ~0;
            gmax = 0;
            gmin_ping = ~0;
            gmax_ping = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

#include <k273/runner.h>

int main(int argc, char** argv) {
    K273::Runner::Config config(argc, argv);
    config.log_filename = "client.log";

    return K273::Runner::Main(go, config);
}

