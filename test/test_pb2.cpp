#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>

#include "iguana/dynamic.hpp"
#include "iguana/pb2_writer.hpp"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/iguana.hpp"
#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
void print_hex_str(const std::string &str) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : str) {
    oss << std::setw(2) << static_cast<int>(c);
  }
  std::cout << oss.str() << std::endl;
}

#define PUBLIC(T) : public iguana::base_impl<T>

struct point_t {
  point_t() = default;
  point_t(int a, double b) : x(a), y(b) {}
  int x;
  double y;
};
YLT_REFL(point_t, x, y);

namespace my_space {
struct inner_struct {
  inner_struct() = default;
  inner_struct(int a, int b, int c) : x(a), y(b), z(c) {}
  int x;
  int y;
  int z;
};

inline auto get_members_impl(inner_struct *) {
  return std::make_tuple(iguana::build_pb_field<&inner_struct::x, 7>("a"),
                         iguana::build_pb_field<&inner_struct::y, 9>("b"),
                         iguana::build_pb_field<&inner_struct::z, 12>("c"));
}
}  // namespace my_space

struct test_pb_st1 {
  test_pb_st1() = default;
  test_pb_st1(int a, iguana::sint32_t b, iguana::sint64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::sint32_t y;
  iguana::sint64_t z;
};
YLT_REFL(test_pb_st1, x, y, z);

struct test_pb_sts PUBLIC(test_pb_sts) {
  test_pb_sts() = default;
  test_pb_sts(std::vector<test_pb_st1> l) : list(std::move(l)) {}
  std::vector<test_pb_st1> list;
};
YLT_REFL(test_pb_sts, list);

struct test_pb_st2 {
  test_pb_st2() = default;
  test_pb_st2(int a, iguana::fixed32_t b, iguana::fixed64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::fixed32_t y;
  iguana::fixed64_t z;
};
YLT_REFL(test_pb_st2, x, y, z);

struct test_pb_st3 {
  test_pb_st3() = default;
  test_pb_st3(int a, iguana::sfixed32_t b, iguana::sfixed64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::sfixed32_t y;
  iguana::sfixed64_t z;
};
YLT_REFL(test_pb_st3, x, y, z);

struct test_pb_st4 {
  test_pb_st4() = default;
  test_pb_st4(int a, std::string b) : x(a), y(std::move(b)) {}
  int x;
  std::string y;
};
YLT_REFL(test_pb_st4, x, y);

struct test_pb_st5 {
  test_pb_st5() = default;
  test_pb_st5(int a, std::string_view b) : x(a), y(b) {}
  int x;
  std::string_view y;
};
YLT_REFL(test_pb_st5, x, y);

struct test_pb_st6 {
  test_pb_st6() = default;
  test_pb_st6(std::optional<int> a, std::optional<std::string> b)
      : x(std::move(a)), y(std::move(b)) {}
  std::optional<int> x;
  std::optional<std::string> y;
};
YLT_REFL(test_pb_st6, x, y);

struct pair_t
    : public iguana::base_impl<pair_t, iguana::ENABLE_XML | iguana::ENABLE_PB |
                                           iguana::ENABLE_YAML |
                                           iguana::ENABLE_JSON> {
  pair_t() = default;
  pair_t(int a, int b) : x(a), y(b) {}
  int x;
  int y;
};
YLT_REFL(pair_t, x, y);

struct message_t PUBLIC(message_t) {
  message_t() = default;
  message_t(int a, pair_t b) : id(a), t(b) {}
  int id;
  pair_t t;
};
YLT_REFL(message_t, id, t);

struct test_pb_st8 {
  test_pb_st8() = default;
  test_pb_st8(int a, pair_t b, message_t c) : x(a), y(b), z(c) {}

  int x;
  pair_t y;
  message_t z;
};
YLT_REFL(test_pb_st8, x, y, z);

struct test_pb_st9 {
  test_pb_st9() = default;
  test_pb_st9(int a, std::vector<int> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<int> y;
  std::string z;
};
YLT_REFL(test_pb_st9, x, y, z);

struct test_pb_st10 {
  test_pb_st10() = default;
  test_pb_st10(int a, std::vector<message_t> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<message_t> y;
  std::string z;
};
YLT_REFL(test_pb_st10, x, y, z);

struct test_pb_st11 PUBLIC(test_pb_st11) {
  test_pb_st11() = default;
  test_pb_st11(int a, std::vector<std::optional<message_t>> b,
               std::vector<std::string> c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<std::optional<message_t>> y;
  std::vector<std::string> z;
};
YLT_REFL(test_pb_st11, x, y, z);

struct test_pb_st12 {
  test_pb_st12() = default;
  test_pb_st12(int a, std::map<int, std::string> b,
               std::map<std::string, int> c)
      : x(a), y(std::move(b)), z(std::move(c)) {}

  int x;
  std::map<int, std::string> y;
  std::map<std::string, int> z;
};
YLT_REFL(test_pb_st12, x, y, z);

struct test_pb_st13 {
  test_pb_st13() = default;
  test_pb_st13(int a, std::map<int, message_t> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}

  int x;
  std::map<int, message_t> y;
  std::string z;
};
YLT_REFL(test_pb_st13, x, y, z);

enum class colors_t { red, black };

enum level_t { debug, info };

struct test_pb_st14 {
  test_pb_st14() = default;
  test_pb_st14(int a, colors_t b, level_t c) : x(a), y(b), z(c) {}
  int x;
  colors_t y;
  level_t z;
};
YLT_REFL(test_pb_st14, x, y, z);

namespace client {
struct person {
  person() = default;
  person(std::string s, int d) : name(s), age(d) {}
  std::string name;
  int64_t age;
};

YLT_REFL(person, name, age);
}  // namespace client

struct my_struct PUBLIC(my_struct) {
  my_struct() = default;
  my_struct(int a, bool b, iguana::fixed64_t c) : x(a), y(b), z(c) {}
  int x;
  bool y;
  iguana::fixed64_t z;
  bool operator==(const my_struct &other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};
YLT_REFL(my_struct, x, y, z);

struct nest1 PUBLIC(nest1) {
  nest1() = default;
  nest1(std::string s, my_struct t, int d)
      : name(std::move(s)), value(t), var(d) {}
  std::string name;
  my_struct value;
  int var;
  std::variant<int, double> mv;
};
YLT_REFL(nest1, name, value, var, mv);

struct numer_st PUBLIC(numer_st) {
  numer_st() = default;
  numer_st(bool x, double y, float z) : a(x), b(y), c(z) {}
  bool a;
  double b;
  float c;
};
YLT_REFL(numer_st, a, b, c);

struct MyPerson : public iguana::base_impl<MyPerson, iguana::ENABLE_JSON> {
  MyPerson() = default;
  MyPerson(std::string s, int d) : name(s), age(d) {}
  std::string name;
  int64_t age;
  bool operator==(const MyPerson &other) const {
    return name == other.name && age == other.age;
  }
};

YLT_REFL(MyPerson, name, age);
struct person PUBLIC(person) {
  person() = default;
  person(int32_t a, std::string b, int c, double d)
      : id(a), name(std::move(b)), age(c), salary(d) {}
  int32_t id;
  std::string name;
  int age;
  double salary;
};
YLT_REFL(person, id, name, age, salary);

enum Color { Red = 0, Black = 2, Green = 4 };

namespace iguana {
template <>
struct enum_value<Color> {
  constexpr static std::array<int, 3> value = {0, 2, 4};
};
}  // namespace iguana

struct vector_t {
  int id;
  Color color;
  std::variant<int, pair_t, std::string> variant;
  std::vector<int> ids;
  std::vector<pair_t> pairs;
  std::vector<std::string> strs;
  std::map<std::string, pair_t> map;
  std::string name;
  std::optional<int> op_val;
};
YLT_REFL(vector_t, id, color, variant, ids, pairs, strs, map, name, op_val);

#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
TEST_CASE("struct to proto") {
  /*{
    std::string str;
    iguana::to_proto2<vector_t>(str, "pb");
    std::cout << str;
    CHECK(str.find(R"(syntax = "proto2";)") != std::string::npos);
    CHECK(str.find("message vector_t") != std::string::npos);
    CHECK(str.find("map<string, pair_t>  map = 9;") != std::string::npos);
    CHECK(str.find("Green = 4;") != std::string::npos);

    std::ofstream file("test_vector.proto", std::ios::binary);
    iguana::to_proto2_file<vector_t>(file, "pb");
    file.sync_with_stdio(true);
    file.flush();
    file.close();

    size_t size = std::filesystem::file_size("test_vector.proto");
    std::ifstream in("test_vector.proto", std::ios::binary);
    std::string read_str;
    read_str.resize(size);
    in.read(read_str.data(), size);
    CHECK(read_str.find("map<string, pair_t>  map = 9;") != std::string::npos);
    CHECK(read_str.find("Green = 4;") != std::string::npos);
  }
  {
    std::string str;
    iguana::to_proto2<test_pb_st8>(str, "pb");
    std::cout << str;
    CHECK(str.find("message_t z = 3;") != std::string::npos);
    CHECK(str.find("message message_t") != std::string::npos);
    CHECK(str.find("pair_t t = 2;") != std::string::npos);
    CHECK(str.find("message pair_t") != std::string::npos);
  }
  {
    std::string str;
    iguana::to_proto2<test_pb_st1>(str, "pb");
    std::cout << str;
    CHECK(str.find("sint64 z = 3;") != std::string::npos);

    iguana::to_proto2<test_pb_st2, false>(str);
    std::cout << str;
    CHECK(str.find("fixed64 z = 3;") != std::string::npos);

    iguana::to_proto2<test_pb_st3, false>(str);
    std::cout << str;
    CHECK(str.find("sfixed64 z = 3;") != std::string::npos);

    iguana::to_proto2<message_t, false>(str);
    std::cout << str;
    CHECK(str.find("pair_t t = 2;") != std::string::npos);
  }
  {
    std::string str;
    iguana::to_proto2<person>(str);
    std::cout << str;
    CHECK(str.find("int32 age = 3;") != std::string::npos);
  }*/
}
#endif

TEST_CASE("test struct_pb") {
  {
    my_space::inner_struct inner{41, 42, 43};

    std::string str;
    iguana::to_pb2(inner, str);

    my_space::inner_struct inner1;
    iguana::from_pb2(inner1, str);
    CHECK(inner.x == inner1.x);
    CHECK(inner.y == inner1.y);
    CHECK(inner.z == inner1.z);
  }

  {
    test_pb_st1 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st1 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.x == st2.x);
    CHECK(st1.y.val == st2.y.val);
    CHECK(st1.z.val == st2.z.val);
  }

  {
    test_pb_st2 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st2 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st3 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st3 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st4 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st4 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.y == st2.y);
  }

  {
    test_pb_st5 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st5 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // optional
    test_pb_st6 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st6 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // sub nested objects
    nest1 v{"Hi", {1, false, {3}}, 5}, v2{};
    std::string s;
    iguana::to_pb2(v, s);
    iguana::from_pb2(v2, s);
    CHECK(v.var == v2.var);
    CHECK(v.value.y == v2.value.y);
    CHECK(v.value.z == v2.value.z);

    test_pb_st8 st1{1, {3, 4}, {5, {7, 8}}};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st8 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z.t.y == st2.z.t.y);
  }

  {
    // repeated messages
    test_pb_st9 st1{1, {2, 4, 6}, "test"};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st9 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    test_pb_st10 st1{1, {{5, {7, 8}}, {9, {11, 12}}}, "test"};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st10 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    message_t m{1, {3, 4}};
    test_pb_st11 st1{1, {m}, {}};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st11 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    message_t st1{};
    std::string str;
    iguana::to_pb2(st1, str);

    message_t st2{};
    iguana::from_pb2(st2, str);
    CHECK(st1.id == st2.id);
  }
  {
    test_pb_st11 st1{
        1, {message_t{5, {7, 8}}, message_t{9, {11, 12}}}, {"test"}};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st11 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{1, {{1, "test"}, {2, "ok"}}, {{"test", 4}, {"ok", 6}}};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st12 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{1, {{1, ""}, {0, "ok"}}, {{"", 4}, {"ok", 0}}};
    std::string str;
    iguana::to_pb2(st1, str);  // error
    print_hex_str(str);
    test_pb_st12 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(1, message_t{2, {3, 4}});
    st1.y.emplace(2, message_t{4, {6, 8}});
    st1.z = "test";
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st13 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(2, message_t{});
    st1.y.emplace(3, message_t{});
    st1.z = "test";
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st13 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // enum
    test_pb_st14 st1{1, colors_t::black, level_t::info};
    std::string str;
    iguana::to_pb2(st1, str);

    test_pb_st14 st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // bool float double
    numer_st n{true, 10.25, 4.578}, n1;
    std::string str;
    iguana::to_pb2(n, str);

    iguana::from_pb2(n1, str);
    CHECK(n1.a == n.a);
    CHECK(n1.b == n.b);
    CHECK(n1.c == n.c);
  }
}

TEST_CASE("test members") {
  using namespace iguana;
  using namespace iguana::detail;

  my_space::inner_struct inner{41, 42, 43};
  const auto &map = detail::get_members(inner);
  std::visit(
      [&inner](auto &member) mutable {
        CHECK(member.field_no == 9);
        CHECK(member.field_name == "b");
        CHECK(member.value(inner) == 42);
      },
      map.at(9));

  point_t pt{2, 3};
  const auto &arr1 = detail::get_members(pt);
  auto &val = arr1.at(1);
  std::visit(
      [&pt](auto &member) mutable {
        CHECK(member.field_no == 1);
        CHECK(member.field_name == "x");
        CHECK(member.value(pt) == 2);
      },
      val);
}

struct test_variant {
  test_variant() = default;
  test_variant(int a, std::variant<double, std::string, int> b, double c)
      : x(a), y(std::move(b)), z(c) {}
  int x;
  std::variant<double, std::string, int> y;
  double z;
};
YLT_REFL(test_variant, x, y, z);

TEST_CASE("test variant") {
  // {
  //   constexpr auto tp = detail::get_members_tuple<test_variant>();
  //   static_assert(std::get<0>(tp).field_no == 1);
  //   static_assert(std::get<1>(tp).field_no == 2);
  //   static_assert(std::get<2>(tp).field_no == 3);
  //   static_assert(std::get<3>(tp).field_no == 4);
  //   static_assert(std::get<4>(tp).field_no == 5);
  // }
  // {
  //   constexpr static auto map = detail::get_members<test_variant>();
  //   static_assert(map.find(1) != map.end());
  //   static_assert(map.find(2) != map.end());
  //   static_assert(map.find(3) != map.end());
  //   static_assert(map.find(4) != map.end());
  //   auto val1 = map.find(2);
  //   auto val2 = map.find(3);
  //   std::visit(
  //       [](auto &member) mutable {
  //         CHECK(member.field_no == 2);
  //         CHECK(member.field_name == "y");
  //       },
  //       val1->second);
  //   std::visit(
  //       [](auto &member) mutable {
  //         CHECK(member.field_no == 3);
  //         CHECK(member.field_name == "y");
  //       },
  //       val2->second);
  // }
  {
    test_variant st1 = {5, "Hello, variant!", 3.14};
    std::string str;
    iguana::to_pb2(st1, str);
    test_variant st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<std::string>(st2.y) == "Hello, variant!");
  }
  {
    test_variant st1 = {5, 3.88, 3.14};
    std::string str;
    iguana::to_pb2(st1, str);
    test_variant st2;
    iguana::from_pb2(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<double>(st2.y) == 3.88);
  }
}

struct test_unknown_inner1 {
    int a1;

    iguana::pb2_unknown_view<> unknown_;
};
YLT_REFL(test_unknown_inner1, a1);
REFL_PB2_UNKNOWN_FIELDS(test_unknown_inner1, unknown_)

struct test_unknown_inner11 {
    int a1;
};
YLT_REFL(test_unknown_inner11, a1);

struct test_unknown_inner2 {
    int a1;
    int a2;
    int a3;
};
YLT_REFL(test_unknown_inner2, a1, a2, a3);

struct test_unknown_fields1 {
    int a1;

    iguana::pb2_unknown_view<> unknown_;

    bool operator == (const test_unknown_fields1& other) const {
        return a1 == other.a1;
    }
};
YLT_REFL(test_unknown_fields1, a1);
REFL_PB2_UNKNOWN_FIELDS(test_unknown_fields1, unknown_)

struct test_unknown_fields2 {
    int a1;
    std::string a2;
    std::vector < test_unknown_fields1> a3;
    std::map<std::string, test_unknown_fields1> a4;
    std::variant<int, std::string> a5;
    std::optional<test_unknown_fields1> a6;

    bool operator == (const test_unknown_fields2& other) const {
        return std::tie(a1, a2, a3, a4, a5, a6)
            == std::tie(other.a1, other.a2, other.a3, other.a4, other.a5, other.a6);
    }
};
YLT_REFL(test_unknown_fields2, a1, a2, a3, a4, a5, a6);

TEST_CASE("unknown_fields") {
    test_unknown_fields2 st2;
    st2.a1 = 1;
    st2.a2 = "2";
    st2.a3.emplace_back(test_unknown_fields1{3});
    st2.a4.emplace("4", test_unknown_fields1{5});
    st2.a5 = "6";
    st2.a6.emplace().a1 = 100;
    std::string st2_str;
    iguana::to_pb2(st2, st2_str);

    test_unknown_fields1 st1;
    iguana::from_pb2(st1, st2_str);

    std::string st1_str;
    iguana::to_pb2(st1, st1_str);
    CHECK(st1_str == st2_str);
    
    test_unknown_fields2 st2_1;
    iguana::from_pb2(st2_1, st1_str);

    CHECK(st2 == st2_1);

    test_unknown_inner2 i2;
    i2.a1 = 10;
    i2.a2 = 11;
    i2.a3 = 12;
    std::string i2s;
    iguana::to_pb2(i2, i2s);

    test_unknown_inner1 i1;
    iguana::from_pb2(i1, i2s);

    test_unknown_inner11 i11;
    try {
        // i2s no unknowns fields,  will throw exception
        iguana::from_pb2(i11, i2s);
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

#endif

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP