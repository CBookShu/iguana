#pragma once
#include "pb_util.hpp"


namespace iguana {

    template <typename S = std::string_view>
    struct pb2_unknown_view {
        using field_t = std::pair<uint32_t, S>;
        using arr_t = std::vector<field_t>;
        arr_t unknown_fields_;

        void push(uint32_t key, std::string_view data) {
            if constexpr (std::is_same_v<S, std::string_view>) {
                unknown_fields_.emplace_back(key, data);   
            } else if constexpr(std::is_same_v<S, std::string>) {
                unknown_fields_.emplace_back(key, std::string(data.data(), data.size()));
            } else {
                static_assert(false, "err ....");
            }
        }

        size_t get_unknown_size() const {
            size_t sz = 0;
            for(auto& it:unknown_fields_) {
                sz += detail::variant_uint32_size_constexpr(it.first);
                sz += it.second.size();
            }
            return sz;
        }

        template <typename Writer>
        void write(Writer& writer) const {
            for(auto& p:unknown_fields_) {
                detail::serialize_varint(p.first, writer);  // tag
                writer.write(p.second.data(), p.second.size());
            }
        }
    };

    namespace detail {
        template <typename T>
        constexpr inline WireType get_wire2_type() {
            if constexpr (std::is_integral_v<T> || is_signed_varint_v<T> ||
                std::is_enum_v<T> || std::is_same_v<T, bool>) {
                return WireType::Varint;
            }
            else if constexpr (std::is_same_v<T, fixed32_t> ||
                std::is_same_v<T, sfixed32_t> ||
                std::is_same_v<T, float>) {
                return WireType::Fixed32;
            }
            else if constexpr (std::is_same_v<T, fixed64_t> ||
                std::is_same_v<T, sfixed64_t> ||
                std::is_same_v<T, double>) {
                return WireType::Fixed64;
            }
            else if constexpr (std::is_same_v<T, std::string> ||
                std::is_same_v<T, std::string_view> ||
                ylt_refletable_v<T> || is_map_container<T>::value) {
                return WireType::LengthDelimeted;
            }
            else if constexpr (optional_v<T>) {
                return get_wire2_type<typename T::value_type>();
            }
            else if constexpr (is_sequence_container<T>::value) {
                return get_wire2_type<typename T::value_type>();
            }
            else {
                throw std::runtime_error("unknown type");
            }
        }

        template <typename T>
        constexpr bool is_lenprefix2_v =
            (get_wire2_type<T>() == WireType::LengthDelimeted);

        template <typename T, typename = void>
        struct has_unknown_fields : public std::false_type{};

        // 自定义 refl_get_unknown_fields 函数 返回pb2_unknown_view
        template <typename T>
        struct has_unknown_fields<T, std::void_t<decltype(refl_get_unknown_fields(std::declval<T>()))>> : public std::true_type{};

#define REFL_PB2_UNKNOWN_FIELDS(T, unknowns)    \
        decltype(auto) refl_get_unknown_fields(T& t) {\
            return (t.##unknowns);\
        }\
        decltype(auto) refl_get_unknown_fields(const T& t) {\
            return (t.##unknowns);\
        }

        template <typename T>
        constexpr bool is_has_unknown_fields = has_unknown_fields<std::decay_t<T>>::value;

        template <typename T>
        size_t pb2_get_unknown_size(T&& t) {
            if constexpr(is_has_unknown_fields<T>) {
                return refl_get_unknown_fields(std::forward<T>(t)).get_unknown_size();
            }
            return 0;
        }

        template <typename T, typename It>
        void pb2_write_unknown_fields(T&& t, It& it) {
            if constexpr(is_has_unknown_fields<T>) {
                return refl_get_unknown_fields(t).write(it);
            }
        }

        template <typename T>
        void pb2_push_unknown_fields(T&t, uint32_t key, std::string_view data) {
            if constexpr(is_has_unknown_fields<T>) {
                return refl_get_unknown_fields(t).push(key, data);
            }
        }

        template <size_t key_size, bool omit_default_val = true, typename Type,
            typename Arr>
        IGUANA_INLINE size_t pb2_key_value_size(Type&& t, Arr& size_arr);

        template <size_t field_no, typename Type, typename Arr>
        IGUANA_INLINE size_t pb2_oneof_size(Type&& t, Arr& size_arr) {
            using T = std::decay_t<Type>;
            int len = 0;
            std::visit(
                [&len, &size_arr](auto&& value) IGUANA__INLINE_LAMBDA{
                  using raw_value_type = decltype(value);
                  using value_type =
                      std::remove_const_t<std::remove_reference_t<decltype(value)>>;
                  constexpr auto offset =
                      get_variant_index<T, value_type, std::variant_size_v<T> -1>();
                  constexpr uint32_t key =
                      ((field_no + offset) << 3) |
                      static_cast<uint32_t>(get_wire2_type<value_type>());
                  len = pb2_key_value_size<variant_uint32_size_constexpr(key), false>(
                      std::forward<raw_value_type>(value), size_arr);
                },
                std::forward<Type>(t));
            return len;
        }

        template <size_t key_size, bool omit_default_val, typename Type, typename Arr>
        IGUANA_INLINE size_t pb2_key_value_size(Type&& t, Arr& size_arr) {
            using T = std::remove_const_t<std::remove_reference_t<Type>>;
            if constexpr (ylt_refletable_v<T> || is_custom_reflection_v<T>) {
                size_t len = 0;
                static auto tuple = get_pb_members_tuple(std::forward<Type>(t));
                constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
                size_t pre_index = -1;
                if constexpr (!inherits_from_base_v<T> && key_size != 0) {
                    pre_index = size_arr.size();
                    size_arr.push_back(0);  // placeholder
                }
                for_each_n(
                    [&len, &t, &size_arr](auto i) IGUANA__INLINE_LAMBDA{
                      using field_type =
                          std::tuple_element_t<decltype(i)::value,
                                               std::decay_t<decltype(tuple)>>;
                      auto value = std::get<decltype(i)::value>(tuple);
                      using U = typename field_type::value_type;
                      using sub_type = typename field_type::sub_type;
                      auto & val = value.value(t);
                      if constexpr (variant_v<U>) {
                        constexpr auto offset =
                            get_variant_index<U, sub_type, std::variant_size_v<U> -1>();
                        if constexpr (offset == 0) {
                          len += pb2_oneof_size<value.field_no>(val, size_arr);
                        }
                      }
                      else {
                        constexpr uint32_t sub_key =
                            (value.field_no << 3) |
                            static_cast<uint32_t>(get_wire2_type<U>());
                        constexpr auto sub_keysize = variant_uint32_size_constexpr(sub_key);
                        len += pb2_key_value_size<sub_keysize, omit_default_val>(val,
                                                                                 size_arr);
                      }
                    },
                    std::make_index_sequence<SIZE>{});
                len += detail::pb2_get_unknown_size(t);
                if constexpr (inherits_from_base_v<T>) {
                    t.cache_size = len;
                }
                else if constexpr (key_size != 0) {
                    size_arr[pre_index] = len;
                }
                if constexpr (key_size == 0) {
                    // for top level
                    return len;
                }
                else {
                    if (len == 0) {
                        // equals key_size  + variant_uint32_size(len)
                        return key_size + 1;
                    }
                    else {
                        return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
                    }
                }
            }
            else if constexpr (is_sequence_container<T>::value) {
                size_t len = 0;
                for (auto& item : t) {
                    len += pb2_key_value_size<key_size, false>(item, size_arr);
                }
                return len;
            }
            else if constexpr (is_map_container<T>::value) {
                size_t len = 0;
                for (auto& [k, v] : t) {
                    // the key_size of  k and v  is constant 1
                    auto kv_len = pb2_key_value_size<1, false>(k, size_arr) +
                        pb2_key_value_size<1, false>(v, size_arr);
                    len += key_size + variant_uint32_size(static_cast<uint32_t>(kv_len)) +
                        kv_len;
                }
                return len;
            }
            else if constexpr (optional_v<T>) {
                if (!t.has_value()) {
                    return 0;
                }
                return pb2_key_value_size<key_size, omit_default_val>(*t, size_arr);
            }
            else {
                return str_numeric_size<key_size, omit_default_val>(t);
            }
        }

        // return the payload size
        template <bool skip_next = true, typename Type>
        IGUANA_INLINE size_t pb2_value_size(Type&& t, uint32_t*& sz_ptr) {
            using T = std::remove_const_t<std::remove_reference_t<Type>>;
            if constexpr (ylt_refletable_v<T> || is_custom_reflection_v<T>) {
                if constexpr (inherits_from_base_v<T>) {
                    return t.cache_size;
                }
                else {
                    // *sz_ptr is secure and logically guaranteed
                    if constexpr (skip_next) {
                        return *(sz_ptr++);
                    }
                    else {
                        return *sz_ptr;
                    }
                }
            }
            else if constexpr (is_sequence_container<T>::value) {
                using item_type = typename T::value_type;
                size_t len = 0;
                if constexpr (!is_lenprefix2_v<item_type>) {
                    for (auto& item : t) {
                        len += str_numeric_size<0, false>(item);
                    }
                    return len;
                }
                else {
                    static_assert(!sizeof(item_type), "the size of this type is meaningless");
                }
            }
            else if constexpr (is_map_container<T>::value) {
                static_assert(!sizeof(T), "the size of this type is meaningless");
            }
            else if constexpr (optional_v<T>) {
                if (!t.has_value()) {
                    return 0;
                }
                return pb2_value_size(*t, sz_ptr);
            }
            else {
                return str_numeric_size<0, false>(t);
            }
        }

    }  // namespace detail

}  // namespace iguana