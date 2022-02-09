namespace SoulFila
{
    /*
	 * A random access iterator that wraps two other random access iterators.
	 * This mostly exists so that one can sort an array using values from another.
	 */


    template<typename It1, typename It2>
    class Zip2Iterator {
        std::pair<It1, It2> mIt;
        using Ref1 = typename std::iterator_traits<It1>::reference;
        using Ref2 = typename std::iterator_traits<It2>::reference;
        using Val1 = typename std::iterator_traits<It1>::value_type;
        using Val2 = typename std::iterator_traits<It2>::value_type;

    public:
        struct Ref : public std::pair<Ref1, Ref2> {
            using std::pair<Ref1, Ref2>::pair;
            using std::pair<Ref1, Ref2>::operator=;
        private:
            friend void swap(Ref lhs, Ref rhs) {
                using std::swap;
                swap(lhs.first, rhs.first);
                swap(lhs.second, rhs.second);
            }
        };

        using value_type = std::pair<Val1, Val2>;
        using reference = Ref;
        using pointer = value_type*;
        using difference_type = ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

        Zip2Iterator() = default;
        Zip2Iterator(It1 first, It2 second) : mIt({ first, second }) {}
        Zip2Iterator(Zip2Iterator const& rhs) noexcept = default;
        Zip2Iterator& operator=(Zip2Iterator const& rhs) = default;

        reference operator*() const { return { *mIt.first, *mIt.second }; }

        reference operator[](size_t n) const { return *(*this + n); }

        Zip2Iterator& operator++() {
            ++mIt.first;
            ++mIt.second;
            return *this;
        }

        Zip2Iterator& operator--() {
            --mIt.first;
            --mIt.second;
            return *this;
        }

        // Postfix operator needed by Microsoft C++
        const Zip2Iterator operator++(int) {
            Zip2Iterator t(*this);
            mIt.first++;
            mIt.second++;
            return t;
        }

        const Zip2Iterator operator--(int) {
            Zip2Iterator t(*this);
            mIt.first--;
            mIt.second--;
            return t;
        }

        Zip2Iterator& operator+=(size_t v) {
            mIt.first += v;
            mIt.second += v;
            return *this;
        }

        Zip2Iterator& operator-=(size_t v) {
            mIt.first -= v;
            mIt.second -= v;
            return *this;
        }

        Zip2Iterator operator+(size_t rhs) const { return { mIt.first + rhs, mIt.second + rhs }; }
        Zip2Iterator operator+(size_t rhs) { return { mIt.first + rhs, mIt.second + rhs }; }
        Zip2Iterator operator-(size_t rhs) const { return { mIt.first - rhs, mIt.second - rhs }; }

        difference_type operator-(Zip2Iterator const& rhs) const { return mIt.first - rhs.mIt.first; }

        bool operator==(Zip2Iterator const& rhs) const { return (mIt.first == rhs.mIt.first); }
        bool operator!=(Zip2Iterator const& rhs) const { return (mIt.first != rhs.mIt.first); }
        bool operator>=(Zip2Iterator const& rhs) const { return (mIt.first >= rhs.mIt.first); }
        bool operator> (Zip2Iterator const& rhs) const { return (mIt.first > rhs.mIt.first); }
        bool operator<=(Zip2Iterator const& rhs) const { return (mIt.first <= rhs.mIt.first); }
        bool operator< (Zip2Iterator const& rhs) const { return (mIt.first < rhs.mIt.first); }
    };

}
