namespace K273 {

    template <typename T>
    class Counter {

    public:
        Counter() = default;
        ~Counter() = default;

    public:

        void reset() {
            this->counts->clear();
        }

        void incr(T& key) {
            if (this->counts->find(key) == this->counts->end()) {
                this->counts[key] = 0;
            }

            this->counts[key] += 1;
        }

        void decr(T& key) {
            if (this->counts->find(key) == this->counts->end()) {
                this->counts[key] = 0;
            }

            this->counts[key] -= 1;
        }

    private:
        std::unordered_map <T, int> counts;
    };

}
