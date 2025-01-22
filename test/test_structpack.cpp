#include <filesystem>
#include <fstream>
#include <iguana/struct_pack.hpp>
#include <iostream>
#include <memory>
#include <optional>


#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

struct pack_st {
  int age;
  std::string name;
  std::vector<int> backs;
};

TEST_CASE("struct_pack_test") {
  pack_st st;
  st.age = 10;
  st.name = "test";
  st.backs.push_back(10);
  st.backs.push_back(100);

  auto s = struct_pack::serialize<std::string>(st);

  pack_st st1;
  auto ec = struct_pack::deserialize_to(st1, s);
  CHECK(!ec);

  CHECK(st1.age == st.age);
  CHECK(st1.name == st.name);
  CHECK(st1.backs == st.backs);
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP