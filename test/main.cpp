#include <iostream>
#include <string>
#include "filestdio.hpp"

int main(int argc, const char * argv[])
{
    try
    {
        {
            filestdio::Redirect out("test.txt", filestdio::Stream::out);

            std::cout << "test";
        }

        std::cout << "test2" << std::endl;

        filestdio::Redirect in("test.txt", filestdio::Stream::in);

        std::string s;
        std::cin >> s;

        if (s != "test")
            throw std::runtime_error("Wrong string");
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
