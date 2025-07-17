#ifndef THIRD_PARTY_ODML_LITERT_LM_SCHEMA_CORE_LITERTLM_HEADER_H_
#define THIRD_PARTY_ODML_LITERT_LM_SCHEMA_CORE_LITERTLM_HEADER_H_

#include <cstdint>
#include <string>

#include "flatbuffers/buffer.h"  // from @flatbuffers
#include "flatbuffers/flatbuffer_builder.h"  // from @flatbuffers
#include "schema/core/litertlm_header_schema_generated.h"

namespace litert {

namespace lm {

namespace schema {

// LiteRT-LM File Format Version uses Semantic Version Rules (SemVer):
// MAJOR version: increments for incompatible API changes.
// MINOR version: increments on added functionality in a backward
//                compatible manner.
// PATCH version: increments on backward compatible bug fixes.
constexpr uint32_t LITERTLM_MAJOR_VERSION = 1;
constexpr uint32_t LITERTLM_MINOR_VERSION = 2;
constexpr uint32_t LITERTLM_PATCH_VERSION = 0;

// Alias for a fully constructed KeyValuePair for LiteRTLM metadata.
// Users of the CreateKeyValuePair function (see below) will get
// back one of these during the creation of their metadata
// data structures.
using KVPair = ::flatbuffers::Offset<KeyValuePair>;

template <typename T>
struct ValueTypeTraits {
  using SchemaType = T;
};

template <>
struct ValueTypeTraits<uint8_t> {
  using SchemaType = UInt8;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, uint8_t value) {
    return CreateUInt8(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<int8_t> {
  using SchemaType = Int8;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, int8_t value) {
    return CreateInt8(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<uint16_t> {
  using SchemaType = UInt16;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, uint16_t value) {
    return CreateUInt16(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<int16_t> {
  using SchemaType = Int16;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, int16_t value) {
    return CreateInt16(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<uint32_t> {
  using SchemaType = UInt32;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, uint32_t value) {
    return CreateUInt32(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<int32_t> {
  using SchemaType = Int32;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, int32_t value) {
    return CreateInt32(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<float> {
  using SchemaType = Float32;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, float value) {
    return CreateFloat32(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<bool> {
  using SchemaType = Bool;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, bool value) {
    return CreateBool(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<uint64_t> {
  using SchemaType = UInt64;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, uint64_t value) {
    return CreateUInt64(builder, value).Union();
  }
};
template <>
struct ValueTypeTraits<int64_t> {
  using SchemaType = Int64;
  static flatbuffers::Offset<void> create(
      flatbuffers::FlatBufferBuilder& builder, int64_t value) {
    return CreateInt64(builder, value).Union();
  }
};

template <typename T>
KVPair CreateKeyValuePair(flatbuffers::FlatBufferBuilder& builder,
                          const std::string& key, const T& value) {
  auto key_offset = builder.CreateString(key);

  flatbuffers::Offset<void> value_offset;
  using VData_SchemaType = ValueTypeTraits<T>::SchemaType;
  VData value_type = VDataTraits<VData_SchemaType>::enum_value;
  value_offset = ValueTypeTraits<T>::create(builder, value);
  KeyValuePairBuilder kvp_builder(builder);
  kvp_builder.add_key(key_offset);
  kvp_builder.add_value(value_offset);
  kvp_builder.add_value_type(value_type);
  return kvp_builder.Finish();
}


template <>
inline KVPair CreateKeyValuePair(flatbuffers::FlatBufferBuilder& builder,
                                 const std::string& key,
                                 const std::string& value) {
  auto key_offset = builder.CreateString(key);
  // NB: The StringValue object *must* be created before the
  // KeyValuePairBuilder.
  auto value_offset = CreateStringValue(builder, builder.CreateString(value));
  KeyValuePairBuilder kvp_builder(builder);
  kvp_builder.add_key(key_offset);
  kvp_builder.add_value(value_offset.Union());
  kvp_builder.add_value_type(VData::VData_StringValue);
  return kvp_builder.Finish();
}

template <>
inline KVPair CreateKeyValuePair(
    flatbuffers::FlatBufferBuilder& builder, const std::string& key,
    const flatbuffers::Offset<StringValue>& value) {
  auto key_offset = builder.CreateString(key);
  KeyValuePairBuilder kvp_builder(builder);
  kvp_builder.add_key(key_offset);
  kvp_builder.add_value(value.Union());
  kvp_builder.add_value_type(VData::VData_StringValue);
  return kvp_builder.Finish();
}

}  // end namespace schema
}  // end namespace lm
}  // end namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_SCHEMA_CORE_LITERTLM_HEADER_H_
