#include <iostream>
#include <limits>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>

template <typename T>
class Rand {
    public:
        Rand(T low = 0, //std::numeric_limits<T>::min(), 
             T high = std::numeric_limits<T>::max()) : 
        r(bind(std::uniform_real_distribution<>(low, high), std::mt19937_64( (unsigned int)(time(NULL)) ))) {}
        T operator()() const { return r(); }
    private:
        std::function<T()> r;
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: generate_file.cpp <output file name>" << std::endl;
        return 0;
    }
    std::string filename = argv[1];

    Rand<double> f{};

    size_t max = 0;
    {
        std::stringstream ss;
        ss << f();

        max = 1024 * 1024 * 1024 / (ss.str().size() + 1); // + \n
    }

    std::ofstream out(filename, std::ios::binary);
    for (int i = 0; i < max; ++i)
    {
        out << f();
        out << "\n";
    }

    return 0;
}
