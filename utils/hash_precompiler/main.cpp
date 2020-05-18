#include "jaws/jaws.hpp"
#include "boost/program_options.hpp"

using namespace po = boost::program_options;

int main(int argc, char** argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("compression", po::value<int>(), "set compression level");

    po::variables_map options;
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);    

    if (options.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }
}
