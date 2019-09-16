#pragma once

#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
#include <vector>
#include <condition_variable>

#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)
#define PACKED __attribute__((__packed__))


#pragma once

// std includes
#include <functional>
#include <type_traits>


namespace K273 {

    /*
     * to access c11 enums like int, see:

     * https://stackoverflow.com/questions/8357240/how-to-automatically-convert-strongly-typed-enum-into-int

     */

    template <typename E>
    constexpr auto to_underlying(E e) noexcept {
        return static_cast <std::underlying_type_t<E>>(e);
    }

    template <typename E, typename T>
    constexpr inline typename std::enable_if <std::is_enum<E>::value &&
                                              std::is_integral<T>::value,
                                              E>::type to_enum(T value) noexcept {
        return static_cast <E>(value);
    }

    // from: http://the-witness.net/news/2012/11/scopeexit-in-c11/
    template <typename F>
    class ScopeExit {
        ScopeExit(F f) :
            f(f) {
        }

        ~ScopeExit() {
            f();
        }
    private:
        F f;
    };

    template <typename F>
    ScopeExit<F> makeScopeExit(F f) {
        return ScopeExit<F>(f);
    };

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2

#define SCOPE_EXIT(statement) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = makeScopeExit([=]() {statement;})


    namespace Align {
        // just messing around
        template <typename T>
        T up(T x, T k) {
            T z = (x / k) * k;
            bool cond = x != z;
            return z + cond * k;
        }

        template <typename T>
        T down(T x, T k) {
            return (x / k) * k;
        }
    }

    inline void cpuRelax() {
        __asm__ __volatile__("rep; nop" : : : "memory");
    }

    // read time stamp counter
    inline uint64_t rdtsc() {
        uint32_t hi_ticks, lo_ticks;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo_ticks), "=d" (hi_ticks));
        return (static_cast<uint64_t> (hi_ticks) << 32) | lo_ticks;
    }

    inline double get_rtc_relative_time() {
        /* use this judiciously, for high performance relative time based stuff */
        return rdtsc() / 30000.0;
    }

    inline double get_time() {
        timespec ts;
        if (likely(clock_gettime(CLOCK_REALTIME, &ts) == 0)) {
            return ts.tv_sec + ((double) ts.tv_nsec) / 1000000000.0;
        }

        return -1;
    }

    class Random {
    public:
        Random(unsigned int seed = 0) :
            seed(seed) {

            if (this->seed == 0) {
                timespec ts;
                for (int ii=0; ii<5; ii++) {
                    clock_gettime(CLOCK_REALTIME, &ts);
                    this->seed += ts.tv_nsec;
                }
            }
        }

        unsigned short getShort() {
            this->seed = (214013 * this->seed + 2531011);
            return (this->seed >> 16 & 0x7FFF);
        }

        unsigned short getWithMax(unsigned short max_num) {
            if (unlikely(max_num == 0)) {
                return 0;
            }

            return this->getShort() % max_num;
        }

    private:
        unsigned int seed;
    };

    // this is a thread safe queue...  it is used to communicate back from worker threads, to
    // caller threads safely.  It was this crudest implementation to get things up and running.
    // However, it seems very, very fast...  So no point optimizing for the sake of it, right now.
 #pragma GCC diagnostic ignored "-Wunused-private-field"

    class SpinLock {
    public:
        SpinLock() :
            locked(false) {
        }

        void lock() {
            while (this->locked.exchange(true, std::memory_order_acq_rel)) {
                cpuRelax();
            }
        }

        void unlock() {
            this->locked.store(false, std::memory_order_release);
        }

    private:
        std::atomic <bool> locked;
        char padding_buf[64 - sizeof(std::atomic<bool>)];
    };

 #pragma GCC diagnostic warning "-Wunused-private-field"

    class SpinLockGuard {
    public:
        SpinLockGuard(SpinLock& l):
            spin_lock(l) {
            this->spin_lock.lock();
        }

        ~SpinLockGuard() {
            this->spin_lock.unlock();
        }

    private:
        SpinLock& spin_lock;
    };

    class WorkerThread;

    template <typename T>
    class LockedQueue {
    public:
        void push(T new_value) {
            std::lock_guard <std::mutex> lk(this->mut);
            // SpinLockGuard g(this->spin_lock);
            this->data_queue.push(new_value);
        }

        T pop() {
            std::lock_guard <std::mutex> lk(this->mut);
            // SpinLockGuard g(this->spin_lock);
            if (this->data_queue.empty()) {
                return nullptr;
            }

            T res = this->data_queue.front();
            this->data_queue.pop();
            return res;
        }

        bool empty() {
            std::lock_guard <std::mutex> lk(this->mut);
            // SpinLockGuard g(this->spin_lock);
            return this->data_queue.empty();
        }

    private:
        std::mutex mut;
        //SpinLock spin_lock;
        std::queue <T> data_queue;
    };

    ///////////////////////////////////////////////////////////////////////////////

    class WorkerInterface {
    public:
        WorkerInterface() :
            thread_self(nullptr) {
        }

        virtual ~WorkerInterface() {
        }

    public:
        WorkerThread* getThread() {
            return this->thread_self;
        }

        void setThread(WorkerThread* t) {
            this->thread_self = t;
        }

    public:
        // The interface
        virtual void doWork() = 0;

    protected:
        WorkerThread* thread_self;
    };

    class WorkerThread {
    public:
        WorkerThread(WorkerInterface* worker);
        ~WorkerThread();

    public:
        // interface from main thread:
        void spawn();
        void startPolling();
        void stopPolling();
        void kill();

        void promptWorker();

        // yes this is non const, since only time should get reference when it is not doing work.
        // Up to the callee to call this is the right time (or things will implode).
        WorkerInterface* getWorker() {
            return this->worker;
        }

        // interface from worker thread
        void done() {
            this->worker_ready.exchange(false, std::memory_order_acq_rel);
        }

    private:
        void mainLoop();

    private:
        WorkerInterface* worker;

        std::mutex m;
        std::condition_variable cv;
        bool running;

        std::atomic <bool> poll;
        std::atomic <bool> waiting;
        std::atomic <bool> worker_ready;

        std::unique_ptr<std::thread> the_thread;
    };

template <typename Type_T>  class ObjectPool {
      private:
        struct FreeList {
            FreeList *next;
            Type_T element;
        };

      public:
        ObjectPool(int block_size=100, std::function <void (Type_T*)> init=nullptr) :
            // DEBUG ONLY
            block_size(block_size),
            head_of_free_list(nullptr),
            init_fn(init) {
         }

        ~ObjectPool() {
            // XXX TODO:  track blocks, and delete them...
            // currently leaks
        }

      public:
        // Get a Type_T element from the free list
        template <class ... ArgTypes> Type_T* acquire(ArgTypes ... args) {
            Type_T* pt_new_element;

            // If head_of_free_list is valid, we'll use and move the head to next
            // element in the free list
            if (this->head_of_free_list != nullptr) {
                pt_new_element = &this->head_of_free_list->element;
                this->head_of_free_list = this->head_of_free_list->next;

            } else {
                // The free list is empty (head_of_free_list is nullptr)

                // Allocate a block of memory (calling constructors)
                FreeList* pt_block = new FreeList[this->block_size];

                // XXX just going to assume new is successful...
                if (this->init_fn != nullptr) {
                    for (int ii=0; ii<this->block_size; ii++) {
                        Type_T& element = pt_block[ii].element;
                        this->init_fn(&element);
                    }
                }

                // Set the new element to the first allocated element
                pt_new_element = &pt_block->element;
                pt_block++;

                // we want to set the rest to head_of_free_list and link them up
                this->head_of_free_list = pt_block;

                // Form a new free list by iterating through and linking the chunks
                // together (except for the first one which we have used and and
                // the last which will be null terminated, therefore block_size - 2).

                for (int ii=0; ii < this->block_size - 3; ii++) {
                    FreeList* cur = pt_block++;
                    cur->next = pt_block;
                }

                // Terminate the last one
                pt_block->next = nullptr;
            }

            pt_new_element->acquire(args ...);
            return pt_new_element;
        }

        // Return a Type_T element to the free list
        void release(Type_T* dead_object) {
            // Make returned object head of free list
            FreeList* carcass = (FreeList*) (((char*) dead_object) - sizeof(FreeList*));
            carcass->next = this->head_of_free_list;
            this->head_of_free_list = carcass;
        }

      private:
        int block_size;
        FreeList* head_of_free_list;
        std::function <void (Type_T*)> init_fn;
    };
}
