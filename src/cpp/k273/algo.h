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

    // from stackoverflow :
    // https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-byte
    inline uint8_t reverseByte(uint8_t n) {
        static unsigned char lookup[16] = {
            0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
            0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

        return (lookup[n&0b1111] << 4) | lookup[n>>4];
    }

}
