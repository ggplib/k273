
// k273 includes
#include <k273/util.h>
#include <k273/logging.h>

// std includes
#include <thread>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////

class SimpleWorker : public K273::WorkerInterface {
public:
    SimpleWorker(K273::LockedQueue* done_queue, int identity) :
        identity(identity),
        iterations(0),
        done_queue(done_queue) {
    }

    ~SimpleWorker() {
    }


public:
    void doWork(K273::WorkerThread* thread_self) {
        // do rollout:
        for (int ii=0; ii<10; ii++) {
            this->iterations += this->r.getWithMax(42);
        }

        thread_self->memoryBarrier();
        this->done_queue->push(thread_self);
    }

public:
    int identity;
    int iterations;
    K273::Random r;

private:
    K273::LockedQueue* done_queue;
};

void test_threads(int num_workers) {
    std::vector <K273::WorkerThread*> workers;
    std::queue<K273::WorkerThread *> workers_available;
    K273::LockedQueue done_queue;

    for (int ii=0; ii<num_workers; ii++) {
        SimpleWorker* worker = new SimpleWorker(&done_queue, ii);
        K273::WorkerThread* wt = new K273::WorkerThread(worker);
        wt->spawn();
        workers.push_back(wt);
    }

    K273::l_debug("starting workers");
    for (K273::WorkerThread* wt : workers) {
        workers_available.push(wt);
        wt->startPolling();
    }

    K273::l_debug("doing work");
    double s = K273::get_time();

    int count = 0;
    const int ITERATIONS = 1000000;
    while (count < ITERATIONS) {
        while (!workers_available.empty()) {
            K273::WorkerThread* next = workers_available.front();
            workers_available.pop();
            next->promptWorker();
        }

        while (true) {
            K273::WorkerThread* done_next = done_queue.pop();
            if (done_next == nullptr) {
                break;
            }

            workers_available.push(done_next);
            count++;
        }
    }

    double e = K273::get_time() - s;
    e /= ITERATIONS;
    for (K273::WorkerThread* wt : workers) {
        SimpleWorker* worker = static_cast<SimpleWorker*> (wt->getWorker());
        K273::l_info("DONE %d : %d", count++, worker->iterations);
        wt->kill();

        delete worker;
        delete wt;
    }

    K273::l_info("Time taken %.3f usecs", e * 1000000);
}

///////////////////////////////////////////////////////////////////////////////

void run(std::vector <std::string>& args) {
    K273::l_debug("Hello world\n");

    for (int ii=0; ii<5; ii++) {
        test_threads(3);
    }
}

///////////////////////////////////////////////////////////////////////////////

#include <k273/runner.h>

int main(int argc, char** argv) {
    K273::Runner::Config config(argc, argv);
    config.log_filename = "somefile.log";

    return K273::Runner::Main(run, config);
}
