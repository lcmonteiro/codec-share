#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "stream.hpp"

TEST(codec_shared_stream, positive_test) {
    // auto in = Codec::Container::Frame("test");
    auto is = codec::istream<std::vector<uint8_t>>();
    // auto os = Codec::OShareStream();

    // std::cout << __FILE__ << ":" << __LINE__ << std::endl;

    // for (auto n = is.set(in, 5, 2); n; --n){
    //     std::cout << __FILE__ << ":" << __LINE__ << std::endl;
    //     auto f = is.pop();
    //     std::cout << f << std::endl;
    //     std::cout << __FILE__ << ":" << __LINE__ << std::endl;
    //     if (os.push(is.pop()) != 0)
    //         break;
    // }

    // EXPECT_EQ(os.get(), in);
}
