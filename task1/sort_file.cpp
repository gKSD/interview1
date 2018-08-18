#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <queue>
#include <chrono>

int g_MaxCHunkSize = 1024 * 1024 * 100;

struct MergeData
{
    MergeData(const double &d, std::ifstream *i) :
        data(d),
        stream(i)
    {}

    bool operator<(const MergeData &rhs) const
    {
        return data > rhs.data;
    }

    double data;
    std::ifstream *stream;
};

std::vector<std::string> read_chunks_and_sort(const std::string &path)
{
    std::ifstream ifs(path.c_str());
    if (!ifs.good())
    {
        throw std::runtime_error(std::string("cannot open file: ") + path);
    }

    int k = 0;
    std::vector<std::string> parts;

    bool last_record = false;
    while (!last_record)
    {
        std::vector<double> vals;
        while (1)
        {
            double tmp;
            if (!(ifs >> tmp))
            {
                last_record = true;
                break;
            }

            vals.push_back(tmp);

            if (vals.size() * sizeof(double) >= g_MaxCHunkSize )
            {
                break;
            }
        }

        std::sort(vals.begin(), vals.end());
        std::string part = "part" + std::to_string(k++) + ".txt";
        std::ofstream out(part, std::ios::binary);
        for (int i = 0; i < vals.size(); ++i)
        {
            out << vals[i];
            out << "\n";
        }
        parts.push_back(part);
    }

    return parts;
}

void merge_sort(const std::string &input, const std::string &output)
{
    std::vector<std::string> parts = read_chunks_and_sort(input);

    std::vector<std::ifstream*> ifs;
    ifs.reserve(parts.size());
    for (auto const& part : parts)
    {
        std::ifstream *tmp = new std::ifstream(part.c_str());
        if (!tmp->good())
        {
            throw std::runtime_error(std::string("cannot open file: ") + part);
        }

        ifs.push_back(tmp);
    }

    std::priority_queue<MergeData> merge_data;  // stores data at current stage of merge-sort algorith
    for (int i = 0; i < ifs.size(); ++i)
    {
        double tmp;
        if (!(*ifs[i] >> tmp))
        {
            throw std::runtime_error("error: empty part file");
        }

        merge_data.push(MergeData(tmp, ifs[i]));
    }

    std::ofstream out(output, std::ios::binary);
    while (!merge_data.empty())
    {
        MergeData min = merge_data.top();

        out << min.data;
        out << "\n";

        merge_data.pop();

        double tmp;
        if (!(*min.stream >> tmp))
        {
            // ifs has no values
            continue;
        }
        merge_data.push(MergeData(tmp, min.stream));
    }

    for (int i = 0; i < ifs.size(); ++i)
    {
        delete ifs[i];
    }

    for (int i = 0; i < parts.size(); ++i)
    {
        std::remove(parts[i].c_str());
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: generate_file.cpp <output file name>" << std::endl;
        return 0;
    }
    std::string input = argv[1];
    std::string output = argv[2];

    auto time_begin = std::chrono::steady_clock::now();
    auto tick_start = clock();
    merge_sort(input, output);
    auto tick_end = clock();
    auto time_end = std::chrono::steady_clock::now();
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count();
    int ticks = tick_end - tick_start;
    std::cout << "time: " << ms << "ms, ticks: " << ticks << std::endl;


    return 0;
}
