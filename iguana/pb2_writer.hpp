#pragma once
#include "pb2_util.hpp"
#include "detail/string_resize.hpp"

namespace iguana {
namespace detail {

template <uint32_t key, bool omit_default_val = true, typename Type,
          typename It>
IGUANA_INLINE void to_pb2_impl(Type&& t, It&& it, uint32_t*& sz_ptr,
                               pb_unknown_fields* unknowns);

template <uint32_t key, typename V, typename It>
IGUANA_INLINE void encode_pair_value2(V&& val, It&& it, size_t size,
                                      uint32_t*& sz_ptr,
                                      pb_unknown_fields* unknowns) {
  if (size == 0)
    IGUANA_UNLIKELY {
      // map keys can't be omitted even if values are empty
      // TODO: repeated ?
      serialize_varint_u32_constexpr<key>(it);
      serialize_varint(0, it);
    }
  else {
    to_pb2_impl<key, false>(val, it, sz_ptr, unknowns);
  }
}

template <uint32_t field_no, typename Type, typename It>
IGUANA_INLINE void to_pb2_oneof(Type&& t, It&& it, uint32_t*& sz_ptr,
                                pb_unknown_fields* unknowns) {
  using T = std::decay_t<Type>;
  std::visit(
      [&it, &sz_ptr, &unknowns](auto&& value) IGUANA__INLINE_LAMBDA {
        using raw_value_type = decltype(value);
        using value_type =
            std::remove_const_t<std::remove_reference_t<decltype(value)>>;
        constexpr auto offset =
            get_variant_index<T, value_type, std::variant_size_v<T> - 1>();
        constexpr uint32_t key =
            ((field_no + offset) << 3) |
            static_cast<uint32_t>(get_wire_type<value_type>());
        to_pb2_impl<key, false>(std::forward<raw_value_type>(value), it, sz_ptr,
                                unknowns);
      },
      std::forward<Type>(t));
}


// omit_default_val = true indicates to omit the default value in searlization
template <uint32_t key, bool omit_default_val, typename Type, typename It>
IGUANA_INLINE void to_pb2_impl(Type&& t, It&& it, uint32_t*& sz_ptr,
                               pb_unknown_fields* unknowns) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (ylt_refletable_v<T> || is_custom_reflection_v<T>) {
    // can't be omitted even if values are empty
    if constexpr (key != 0) {
        auto len = pb2_value_size(t, sz_ptr, unknowns);
        serialize_varint_u32_constexpr<key>(it);
        serialize_varint(len, it);
        if (len == 0)
        IGUANA_UNLIKELY { return; }
    }
    static auto tuple = get_pb_members_tuple(std::forward<Type>(t));
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    for_each_n(
        [&t, &it, &sz_ptr, &unknowns](auto i) IGUANA__INLINE_LAMBDA {
          using field_type =
              std::tuple_element_t<decltype(i)::value,
                                   std::decay_t<decltype(tuple)>>;
          auto value = std::get<decltype(i)::value>(tuple);
          auto& val = value.value(t);

          using U = typename field_type::value_type;
          using sub_type = typename field_type::sub_type;
          if constexpr (variant_v<U>) {
            constexpr auto offset =
                get_variant_index<U, sub_type, std::variant_size_v<U> - 1>();
            if constexpr (offset == 0) {
              if (unknowns) {
                to_pb2_oneof<value.field_no>(val, it, sz_ptr,
                                             unknowns->child(value.field_no));
              }
              else {
                to_pb2_oneof<value.field_no>(val, it, sz_ptr, nullptr);
              }
            }
          }
          else {
            constexpr uint32_t sub_key =
                (value.field_no << 3) |
                static_cast<uint32_t>(get_wire2_type<U>());
            if (unknowns) {
              to_pb2_impl<sub_key, omit_default_val>(
                  val, it, sz_ptr, unknowns->child(value.field_no));
            }
            else {
              to_pb2_impl<sub_key, omit_default_val>(val, it, sz_ptr, nullptr);
            }
          }
        },
        std::make_index_sequence<SIZE>{});
    if (unknowns) {
      size_t max_field_no = std::get<SIZE - 1>(tuple).field_no;
      unknowns->write_unknown_field(it, max_field_no);
    }
  }
  else if constexpr (is_sequence_container<T>::value) {
    // pb2 default non-packed
    for (auto& item : t) {
      to_pb2_impl<key, false>(item, it, sz_ptr, unknowns);
    }
  }
  else if constexpr (is_map_container<T>::value) {
    using first_type = typename T::key_type;
    using second_type = typename T::mapped_type;
    constexpr uint32_t key1 =
        (1 << 3) | static_cast<uint32_t>(get_wire2_type<first_type>());
    constexpr auto key1_size = variant_uint32_size_constexpr(key1);
    constexpr uint32_t key2 =
        (2 << 3) | static_cast<uint32_t>(get_wire2_type<second_type>());
    constexpr auto key2_size = variant_uint32_size_constexpr(key2);

    for (auto& [k, v] : t) {
      serialize_varint_u32_constexpr<key>(it);
      // k must be string or numeric
      auto k_val_len = str_numeric_size<0, false>(k);
      auto v_val_len = pb2_value_size<false>(v, sz_ptr, unknowns);
      auto pair_len = key1_size + key2_size + k_val_len + v_val_len;
      if constexpr (is_lenprefix2_v<first_type>) {
        pair_len += variant_uint32_size(k_val_len);
      }
      if constexpr (is_lenprefix2_v<second_type>) {
        pair_len += variant_uint32_size(v_val_len);
      }
      serialize_varint(pair_len, it);
      // map k and v can't be omitted even if values are empty
      encode_pair_value2<key1>(k, it, k_val_len, sz_ptr,unknowns);
      encode_pair_value2<key2>(v, it, v_val_len, sz_ptr,unknowns);
    }
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return;
    }
    to_pb2_impl<key, omit_default_val>(*t, it, sz_ptr, unknowns);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    if constexpr (omit_default_val) {
      if (t.size() == 0)
        IGUANA_UNLIKELY { return; }
    }
    serialize_varint_u32_constexpr<key>(it);
    serialize_varint(t.size(), it);
    memcpy(it, t.data(), t.size());
    it += t.size();
  }
  else {
    encode_numeric_field<key, omit_default_val>(t, it);
  }
}

#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)

template <typename T, typename Map>
IGUANA_INLINE void build_sub_proto2(Map& map, std::string_view str_type,
                                    std::string& sub_str);

template <typename Type, typename Stream>
IGUANA_INLINE void to_proto2_impl(
    Stream& out, std::unordered_map<std::string_view, std::string>& map,
    std::string_view field_name = "", uint32_t field_no = 0, bool repeated = false) {
  std::string sub_str;
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (ylt_refletable_v<T> || is_custom_reflection_v<T>) {
    constexpr auto name = type_string<T>();
    out.append("message ").append(name).append(" {\n");
    static T t;
    static auto tuple = get_pb_members_tuple(t);
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;

    for_each_n(
        [&out, &sub_str, &map](auto i) mutable {
          using field_type =
              std::tuple_element_t<decltype(i)::value,
                                   std::decay_t<decltype(tuple)>>;
          auto value = std::get<decltype(i)::value>(tuple);

          using U = typename field_type::value_type;
          using sub_type = typename field_type::sub_type;
          if constexpr (ylt_refletable_v<U>) {
            constexpr auto str_type = get_type_string<U>();
            build_proto_field(
                out, str_type,
                {value.field_name.data(), value.field_name.size()},
                value.field_no);

            build_sub_proto2<U>(map, str_type, sub_str);
          }
          else if constexpr (variant_v<U>) {
            constexpr size_t var_size = std::variant_size_v<U>;

            constexpr auto offset =
                get_variant_index<U, sub_type, var_size - 1>();

            if (offset == 0) {
              out.append("  oneof ");
              out.append(value.field_name.data(), value.field_name.size())
                  .append(" {\n");
            }

            constexpr auto str_type = get_type_string<sub_type>();
            std::string field_name = " one_of_";
            field_name.append(str_type);

            out.append("  ");
            build_proto_field(out, str_type, field_name, value.field_no);

            if constexpr (ylt_refletable_v<sub_type>) {
              build_sub_proto2<sub_type>(map, str_type, sub_str);
            }

            if (offset == var_size - 1) {
              out.append("  }\n");
            }
          }
          else {
            to_proto2_impl<U>(
                out, map, {value.field_name.data(), value.field_name.size()},
                value.field_no);
          }
        },
        std::make_index_sequence<SIZE>{});
    out.append("}\r\n\r\n");
  }
  else if constexpr (is_sequence_container<T>::value) {
    out.append("  repeated");
    using item_type = typename T::value_type;

    if constexpr (is_lenprefix_v<item_type>) {
      // non-packed
      if constexpr (ylt_refletable_v<item_type>) {
        constexpr auto str_type = get_type_string<item_type>();
        build_proto_field(out, str_type, field_name, field_no);

        build_sub_proto2<item_type>(map, str_type, sub_str);
      }
      else {
        to_proto2_impl<item_type>(out, map, field_name, field_no, true);
      }
    }
    else {
      out.append("  ");
      numeric_to_proto<item_type>(out, field_name, field_no);
    }
  }
  else if constexpr (is_map_container<T>::value) {
    out.append("  map<");
    using first_type = typename T::key_type;
    using second_type = typename T::mapped_type;

    constexpr auto str_first = get_type_string<first_type>();
    constexpr auto str_second = get_type_string<second_type>();
    out.append(str_first).append(", ").append(str_second).append(">");

    build_proto_field<1>(out, "", field_name, field_no);

    if constexpr (ylt_refletable_v<second_type>) {
      constexpr auto str_type = get_type_string<second_type>();
      build_sub_proto2<second_type>(map, str_type, sub_str);
    }
  }
  else if constexpr (optional_v<T>) {
    to_proto2_impl<typename T::value_type>(
        out, map, {field_name.data(), field_name.size()}, field_no);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    if (!repeated) {
      out.append("  optional");
    }
    build_proto_field(out, "string ", field_name, field_no);
  }
  else if constexpr (enum_v<T>) {
    if (!repeated) {
      out.append("  optional");
    }
    constexpr auto str_type = get_type_string<T>();
    static constexpr auto enum_to_str = get_enum_map<false, std::decay_t<T>>();
    if constexpr (bool_v<decltype(enum_to_str)>) {
      build_proto_field(out, "int32", field_name, field_no);
    }
    else {
      static_assert(enum_to_str.size() > 0, "empty enum not allowed");
      static_assert((int)(enum_to_str.begin()->first) == 0,
                    "the first enum value must be zero in proto3");
      build_proto_field(out, str_type, field_name, field_no);
      if (map.find(str_type) == map.end()) {
        sub_str.append("enum ").append(str_type).append(" {\n");
        for (auto& [k, field_name] : enum_to_str) {
          std::string_view name{field_name.data(), field_name.size()};
          size_t pos = name.rfind("::");
          if (pos != std::string_view::npos) {
            name = name.substr(pos + 2);
          }
          sub_str.append("  ")
              .append(name)
              .append(" = ")
              .append(std::to_string(static_cast<std::underlying_type_t<T>>(k)))
              .append(";\n");
        }
        sub_str.append("}\r\n\r\n");
        map.emplace(str_type, std::move(sub_str));
      }
    }
  }
  else {
    if (!repeated) {
      out.append("  optional  ");
    }
    else {
      out.append("  ");
    }
    numeric_to_proto<Type>(out, field_name, field_no);
  }
}

template <typename T, typename Map>
IGUANA_INLINE void build_sub_proto2(Map& map, std::string_view str_type,
                                    std::string& sub_str) {
  if (map.find(str_type) == map.end()) {
    to_proto2_impl<T>(sub_str, map);
    map.emplace(str_type, std::move(sub_str));
  }
}
#endif

}  // namespace detail

template <
    typename T, typename Stream,
    std::enable_if_t<ylt_refletable_v<T> || detail::is_custom_reflection_v<T>,
                     int> = 0>
IGUANA_INLINE void to_pb2(T const& t, Stream& out, pb_unknown_fields* unknowns = nullptr) {
  std::vector<uint32_t> size_arr;
  auto byte_len = detail::pb2_key_value_size<0, false>(t, size_arr);
  if (unknowns) {
    byte_len += unknowns->get_unknown_size();
  }
  detail::resize(out, byte_len);
  auto sz_ptr = size_arr.empty() ? nullptr : &size_arr[0];
  detail::to_pb2_impl<0, false>(t, &out[0], sz_ptr, unknowns);
}

#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
template <typename T, bool gen_header = true, typename Stream>
IGUANA_INLINE void to_proto2(Stream& out, std::string_view ns = "") {
  if (gen_header) {
    constexpr std::string_view crlf = "\r\n\r\n";
    out.append(R"(syntax = "proto2";)").append(crlf);
    if (!ns.empty()) {
      out.append("package ").append(ns).append(";").append(crlf);
    }

    out.append(R"(option optimize_for = SPEED;)").append(crlf);
    out.append(R"(option cc_enable_arenas = true;)").append(crlf);
  }

  std::unordered_map<std::string_view, std::string> map;
  detail::to_proto2_impl<T>(out, map);
  for (auto& [k, s] : map) {
    out.append(s);
  }
}

template <typename T, bool gen_header = true, typename Stream>
IGUANA_INLINE void to_proto2_file(Stream& stream, std::string_view ns = "") {
  if (!stream.is_open()) {
    return;
  }
  std::string out;
  to_proto2<T, gen_header>(out, ns);
  stream.write(out.data(), out.size());
}
#endif

}  // namespace iguana