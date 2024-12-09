#pragma once
#include "pb2_util.hpp"
#include "pb_reader.hpp"

namespace iguana {
template <typename T>
IGUANA_INLINE void from_pb2(T& t, std::string_view pb_str);
namespace detail {
template <typename T>
IGUANA_INLINE void from_pb2_impl(T& val, std::string_view& pb_str,
                                 uint32_t field_no = 0);

template <typename T>
IGUANA_INLINE void decode2_pair_value(T& val, std::string_view& pb_str) {
  size_t pos;
  uint32_t key = detail::decode_varint(pb_str, pos);
  pb_str = pb_str.substr(pos);
  WireType wire_type = static_cast<WireType>(key & 0b0111);
  if (wire_type != detail::get_wire2_type<std::remove_reference_t<T>>()) {
    return;
  }
  from_pb2_impl(val, pb_str, 0);
}

template <typename T, typename Field>
IGUANA_INLINE void parse_oneof2(T& t, const Field& f, std::string_view& pb_str) {
  using item_type = typename std::decay_t<Field>::sub_type;
  from_pb2_impl(t.template emplace<item_type>(), pb_str, f.field_no);
}

template <typename T>
IGUANA_INLINE void from_pb2_impl(T& val, std::string_view& pb_str,
                                 uint32_t field_no) {
  size_t pos = 0;
  if constexpr (ylt_refletable_v<T>) {
    size_t pos;
    uint32_t size = detail::decode_varint(pb_str, pos);
    pb_str = pb_str.substr(pos);
    if (pb_str.size() < size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
    if (size == 0) {
      return;
    }

    from_pb2(val, pb_str.substr(0, size));

    pb_str = pb_str.substr(size);
  }
  else if constexpr (is_sequence_container<T>::value) {
    // item_type non-packed
    using item_type = typename T::value_type;
    while (!pb_str.empty()) {
      item_type item{};
      from_pb2_impl(item, pb_str, 0);
      val.push_back(std::move(item));
      if (pb_str.empty()) {
        break;
      }
      uint32_t key = detail::decode_varint(pb_str, pos);
      uint32_t field_number = key >> 3;
      if (field_number != field_no) {
        break;
      }
      else {
        pb_str = pb_str.substr(pos);
      }
    }
  }
  else if constexpr (is_map_container<T>::value) {
    using item_type = std::pair<typename T::key_type, typename T::mapped_type>;
    while (!pb_str.empty()) {
      size_t pos;
      uint32_t size = detail::decode_varint(pb_str, pos);
      pb_str = pb_str.substr(pos);
      if (pb_str.size() < size)
        IGUANA_UNLIKELY {
          throw std::invalid_argument(
              "Invalid fixed int value: too few bytes.");
        }
      item_type item = {};
      decode2_pair_value(item.first, pb_str);
      decode2_pair_value(item.second, pb_str);
      val.emplace(std::move(item));

      if (pb_str.empty()) {
        break;
      }
      uint32_t key = detail::decode_varint(pb_str, pos);
      uint32_t field_number = key >> 3;
      if (field_number != field_no) {
        break;
      }
      pb_str = pb_str.substr(pos);
    }
  }
  else if constexpr (std::is_integral_v<T>) {
    val = static_cast<T>(detail::decode_varint(pb_str, pos));
    pb_str = pb_str.substr(pos);
  }
  else if constexpr (detail::is_signed_varint_v<T>) {
    constexpr size_t len = sizeof(typename T::value_type);
    uint64_t temp = detail::decode_varint(pb_str, pos);
    if constexpr (len == 8) {
      val.val = detail::decode_zigzag(temp);
    }
    else {
      val.val = detail::decode_zigzag(static_cast<uint32_t>(temp));
    }
    pb_str = pb_str.substr(pos);
  }
  else if constexpr (detail::is_fixed_v<T>) {
    constexpr size_t size = sizeof(typename T::value_type);
    if (pb_str.size() < size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
    memcpy(&(val.val), pb_str.data(), size);
    pb_str = pb_str.substr(size);
  }
  else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
    constexpr size_t size = sizeof(T);
    if (pb_str.size() < size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
    memcpy(&(val), pb_str.data(), size);
    pb_str = pb_str.substr(size);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    size_t size = detail::decode_varint(pb_str, pos);
    if (pb_str.size() < pos + size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid string value: too few bytes.");
      }
    if constexpr (std::is_same_v<T, std::string_view>) {
      val = std::string_view(pb_str.data() + pos, size);
    }
    else {
      detail::resize(val, size);
      memcpy(val.data(), pb_str.data() + pos, size);
    }
    pb_str = pb_str.substr(size + pos);
  }
  else if constexpr (std::is_enum_v<T>) {
    using U = std::underlying_type_t<T>;
    U value{};
    from_pb2_impl(value, pb_str, 0);
    val = static_cast<T>(value);
  }
  else if constexpr (optional_v<T>) {
    from_pb2_impl(val.emplace(), pb_str, 0);
  }
  else {
    static_assert(!sizeof(T), "err");
  }
}
}  // namespace detail

template <typename T>
IGUANA_INLINE void from_pb2(T& t, std::string_view pb_str) {
  if (pb_str.empty())
    IGUANA_UNLIKELY { return; }
  size_t pos = 0;
  uint32_t key = detail::decode_varint(pb_str, pos);
  WireType wire_type = static_cast<WireType>(key & 0b0111);
  uint32_t field_number = key >> 3;
  while (true) {
    pb_str = pb_str.substr(pos);
    static auto map = detail::get_members(std::forward<T>(t));
    auto it = map.find(field_number);
    if (it != map.end()) {
      auto& member = map.at(field_number);
      std::visit(
          [&t, &pb_str, wire_type, key, field_number,
           &member](auto& val) {
            using sub_type = typename std::decay_t<decltype(val)>::sub_type;
            using value_type = typename std::decay_t<decltype(val)>::value_type;
            if (wire_type != detail::get_wire2_type<sub_type>()) {
              throw std::runtime_error("unmatched wire_type");
            }
            if constexpr (variant_v<value_type>) {
              detail::parse_oneof2(val.value(t), val, pb_str);
            }
            else {
              detail::from_pb2_impl(val.value(t), pb_str, val.field_no);
            }
          },
          member);
      if (!pb_str.empty())
        IGUANA_LIKELY {
          key = detail::decode_varint(pb_str, pos);
          wire_type = static_cast<WireType>(key & 0b0111);
          field_number = key >> 3;
        }
      else {
        return;
      }
    }
    else {
      // TODO: skip field no and fresh pos
      switch (wire_type) {
        case WireType::Varint: {
          detail::decode_varint(pb_str, pos);
          if constexpr (detail::is_has_unknown_fields<T>) {
            detail::pb2_push_unknown_fields(t, key, pb_str.substr(0, pos));
          }
        } break;
        case WireType::Fixed64:
          pos = 8;
          if constexpr (detail::is_has_unknown_fields<T>) {
            detail::pb2_push_unknown_fields(t, key, pb_str.substr(0, pos));
          }
          break;
        case WireType::LengthDelimeted: {
          auto sz = detail::decode_varint(pb_str, pos);
          if (pb_str.size() < (pos + sz)) {
            throw std::runtime_error("string len err");
          }
          if constexpr (detail::is_has_unknown_fields<T>) {
            auto s = pb_str.substr(0, sz + pos);
            detail::pb2_push_unknown_fields(t, key, s);
          }
          pos += sz;
        } break;
        case WireType::Fixed32:
          pos = 4;
          if constexpr (detail::is_has_unknown_fields<T>) {
            detail::pb2_push_unknown_fields(t, key, pb_str.substr(0, pos));
          }
          break;
        default:
          throw std::runtime_error("err wire_type");
      }
      pb_str = pb_str.substr(pos);
      if (!pb_str.empty()) {
        IGUANA_LIKELY {
          key = detail::decode_varint(pb_str, pos);
          wire_type = static_cast<WireType>(key & 0b0111);
          field_number = key >> 3;
        }
      }
      else {
        return;
      }
    }
  }
}

}  // namespace iguana