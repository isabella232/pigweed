// Copyright 2020 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#include <span>

#include "gtest/gtest.h"
#include "pw_protobuf/encoder.h"
#include "pw_stream/memory_stream.h"

// These header files contain the code generated by the pw_protobuf plugin.
// They are re-generated every time the tests are built and are used by the
// tests to ensure that the interface remains consistent.
//
// The purpose of the tests in this file is primarily to verify that the
// generated C++ interface is valid rather than the correctness of the
// low-level encoder.
#include "pw_protobuf_test_protos/full_test.pwpb.h"
#include "pw_protobuf_test_protos/importer.pwpb.h"
#include "pw_protobuf_test_protos/non_pw_package.pwpb.h"
#include "pw_protobuf_test_protos/proto2.pwpb.h"
#include "pw_protobuf_test_protos/repeated.pwpb.h"

namespace pw::protobuf {
namespace {

using namespace pw::protobuf::test;

TEST(Codegen, Codegen) {
  std::byte encode_buffer[512];
  std::byte temp_buffer[512];
  stream::MemoryWriter writer(encode_buffer);

  Pigweed::StreamEncoder pigweed(writer, temp_buffer);
  pigweed.WriteMagicNumber(73);
  pigweed.WriteZiggy(-111);
  pigweed.WriteErrorMessage("not a typewriter");
  pigweed.WriteBin(Pigweed::Protobuf::Binary::ZERO);

  {
    Pigweed::Pigweed::StreamEncoder pigweed_pigweed =
        pigweed.GetPigweedEncoder();
    pigweed_pigweed.WriteStatus(Bool::FILE_NOT_FOUND);

    ASSERT_EQ(pigweed_pigweed.status(), OkStatus());
  }

  {
    Proto::StreamEncoder proto = pigweed.GetProtoEncoder();
    proto.WriteBin(Proto::Binary::OFF);
    proto.WritePigweedPigweedBin(Pigweed::Pigweed::Binary::ZERO);
    proto.WritePigweedProtobufBin(Pigweed::Protobuf::Binary::ZERO);

    {
      Pigweed::Protobuf::Compiler::StreamEncoder meta = proto.GetMetaEncoder();
      meta.WriteFileName("/etc/passwd");
      meta.WriteStatus(Pigweed::Protobuf::Compiler::Status::FUBAR);
    }

    {
      Pigweed::StreamEncoder nested_pigweed = proto.GetPigweedEncoder();
      nested_pigweed.WriteErrorMessage("here we go again");
      nested_pigweed.WriteMagicNumber(616);

      {
        DeviceInfo::StreamEncoder device_info =
            nested_pigweed.GetDeviceInfoEncoder();

        {
          KeyValuePair::StreamEncoder attributes =
              device_info.GetAttributesEncoder();
          attributes.WriteKey("version");
          attributes.WriteValue("5.3.1");
        }

        {
          KeyValuePair::StreamEncoder attributes =
              device_info.GetAttributesEncoder();
          attributes.WriteKey("chip");
          attributes.WriteValue("left-soc");
        }

        device_info.WriteStatus(DeviceInfo::DeviceStatus::PANIC);
      }
    }
  }

  for (int i = 0; i < 5; ++i) {
    Proto::ID::StreamEncoder id = pigweed.GetIdEncoder();
    id.WriteId(5 * i * i + 3 * i + 49);
  }

  // clang-format off
  constexpr uint8_t expected_proto[] = {
    // pigweed.magic_number
    0x08, 0x49,
    // pigweed.ziggy
    0x10, 0xdd, 0x01,
    // pigweed.error_message
    0x2a, 0x10, 'n', 'o', 't', ' ', 'a', ' ',
    't', 'y', 'p', 'e', 'w', 'r', 'i', 't', 'e', 'r',
    // pigweed.bin
    0x40, 0x01,
    // pigweed.pigweed
    0x3a, 0x02,
    // pigweed.pigweed.status
    0x08, 0x02,
    // pigweed.proto
    0x4a, 0x56,
    // pigweed.proto.bin
    0x10, 0x00,
    // pigweed.proto.pigweed_pigweed_bin
    0x18, 0x00,
    // pigweed.proto.pigweed_protobuf_bin
    0x20, 0x01,
    // pigweed.proto.meta
    0x2a, 0x0f,
    // pigweed.proto.meta.file_name
    0x0a, 0x0b, '/', 'e', 't', 'c', '/', 'p', 'a', 's', 's', 'w', 'd',
    // pigweed.proto.meta.status
    0x10, 0x02,
    // pigweed.proto.nested_pigweed
    0x0a, 0x3d,
    // pigweed.proto.nested_pigweed.error_message
    0x2a, 0x10, 'h', 'e', 'r', 'e', ' ', 'w', 'e', ' ',
    'g', 'o', ' ', 'a', 'g', 'a', 'i', 'n',
    // pigweed.proto.nested_pigweed.magic_number
    0x08, 0xe8, 0x04,
    // pigweed.proto.nested_pigweed.device_info
    0x32, 0x26,
    // pigweed.proto.nested_pigweed.device_info.attributes[0]
    0x22, 0x10,
    // pigweed.proto.nested_pigweed.device_info.attributes[0].key
    0x0a, 0x07, 'v', 'e', 'r', 's', 'i', 'o', 'n',
    // pigweed.proto.nested_pigweed.device_info.attributes[0].value
    0x12, 0x05, '5', '.', '3', '.', '1',
    // pigweed.proto.nested_pigweed.device_info.attributes[1]
    0x22, 0x10,
    // pigweed.proto.nested_pigweed.device_info.attributes[1].key
    0x0a, 0x04, 'c', 'h', 'i', 'p',
    // pigweed.proto.nested_pigweed.device_info.attributes[1].value
    0x12, 0x08, 'l', 'e', 'f', 't', '-', 's', 'o', 'c',
    // pigweed.proto.nested_pigweed.device_info.status
    0x18, 0x03,
    // pigweed.id[0]
    0x52, 0x02,
    // pigweed.id[0].id
    0x08, 0x31,
    // pigweed.id[1]
    0x52, 0x02,
    // pigweed.id[1].id
    0x08, 0x39,
    // pigweed.id[2]
    0x52, 0x02,
    // pigweed.id[2].id
    0x08, 0x4b,
    // pigweed.id[3]
    0x52, 0x02,
    // pigweed.id[3].id
    0x08, 0x67,
    // pigweed.id[4]
    0x52, 0x03,
    // pigweed.id[4].id
    0x08, 0x8d, 0x01
  };
  // clang-format on

  ConstByteSpan result = writer.WrittenData();
  ASSERT_EQ(pigweed.status(), OkStatus());
  EXPECT_EQ(result.size(), sizeof(expected_proto));
  EXPECT_EQ(std::memcmp(result.data(), expected_proto, sizeof(expected_proto)),
            0);
}

TEST(Codegen, RecursiveSubmessage) {
  std::byte encode_buffer[512];
  NestedEncoder<20, 20> encoder(encode_buffer);

  Crate::Encoder biggest_crate(&encoder);
  biggest_crate.WriteName("Huge crate");

  {
    Crate::Encoder medium_crate = biggest_crate.GetSmallerCratesEncoder();
    medium_crate.WriteName("Medium crate");
    {
      Crate::Encoder small_crate = medium_crate.GetSmallerCratesEncoder();
      small_crate.WriteName("Small crate");
    }
    {
      Crate::Encoder tiny_crate = medium_crate.GetSmallerCratesEncoder();
      tiny_crate.WriteName("Tiny crate");
    }
  }

  // clang-format off
  constexpr uint8_t expected_proto[] = {
    // crate.name
    0x0a, 0x0a, 'H', 'u', 'g', 'e', ' ', 'c', 'r', 'a', 't', 'e',
    // crate.smaller_crate[0]
    0x12, 0x2b,
    // crate.smaller_crate[0].name
    0x0a, 0x0c, 'M', 'e', 'd', 'i', 'u', 'm', ' ', 'c', 'r', 'a', 't', 'e',
    // crate.smaller_crate[0].smaller_crate[0]
    0x12, 0x0d,
    // crate.smaller_crate[0].smaller_crate[0].name
    0x0a, 0x0b, 'S', 'm', 'a', 'l', 'l', ' ', 'c', 'r', 'a', 't', 'e',
    // crate.smaller_crate[0].smaller_crate[1]
    0x12, 0x0c,
    // crate.smaller_crate[0].smaller_crate[1].name
    0x0a, 0x0a, 'T', 'i', 'n', 'y', ' ', 'c', 'r', 'a', 't', 'e',
  };
  // clang-format on

  Result result = encoder.Encode();
  ASSERT_EQ(result.status(), OkStatus());
  EXPECT_EQ(result.value().size(), sizeof(expected_proto));
  EXPECT_EQ(std::memcmp(
                result.value().data(), expected_proto, sizeof(expected_proto)),
            0);
}

TEST(CodegenRepeated, NonPackedScalar) {
  std::byte encode_buffer[32];
  NestedEncoder encoder(encode_buffer);

  RepeatedTest::Encoder repeated_test(&encoder);
  for (int i = 0; i < 4; ++i) {
    repeated_test.WriteUint32s(i * 16);
  }

  constexpr uint8_t expected_proto[] = {
      0x08, 0x00, 0x08, 0x10, 0x08, 0x20, 0x08, 0x30};

  Result result = encoder.Encode();
  ASSERT_EQ(result.status(), OkStatus());
  EXPECT_EQ(result.value().size(), sizeof(expected_proto));
  EXPECT_EQ(std::memcmp(
                result.value().data(), expected_proto, sizeof(expected_proto)),
            0);
}

TEST(CodegenRepeated, PackedScalar) {
  std::byte encode_buffer[32];
  NestedEncoder encoder(encode_buffer);

  RepeatedTest::Encoder repeated_test(&encoder);
  constexpr uint32_t values[] = {0, 16, 32, 48};
  repeated_test.WriteUint32s(values);

  constexpr uint8_t expected_proto[] = {0x0a, 0x04, 0x00, 0x10, 0x20, 0x30};
  Result result = encoder.Encode();
  ASSERT_EQ(result.status(), OkStatus());
  EXPECT_EQ(result.value().size(), sizeof(expected_proto));
  EXPECT_EQ(std::memcmp(
                result.value().data(), expected_proto, sizeof(expected_proto)),
            0);
}

TEST(CodegenRepeated, NonScalar) {
  std::byte encode_buffer[32];
  RepeatedTest::RamEncoder repeated_test(encode_buffer);
  constexpr const char* strings[] = {"the", "quick", "brown", "fox"};
  for (const char* s : strings) {
    repeated_test.WriteStrings(s);
  }

  constexpr uint8_t expected_proto[] = {
      0x1a, 0x03, 't', 'h', 'e', 0x1a, 0x5, 'q',  'u', 'i', 'c', 'k',
      0x1a, 0x5,  'b', 'r', 'o', 'w',  'n', 0x1a, 0x3, 'f', 'o', 'x'};
  ConstByteSpan result(repeated_test);
  ASSERT_EQ(repeated_test.status(), OkStatus());
  EXPECT_EQ(result.size(), sizeof(expected_proto));
  EXPECT_EQ(std::memcmp(result.data(), expected_proto, sizeof(expected_proto)),
            0);
}

TEST(CodegenRepeated, Message) {
  std::byte encode_buffer[64];
  NestedEncoder<1, 3> encoder(encode_buffer);

  RepeatedTest::Encoder repeated_test(&encoder);
  for (int i = 0; i < 3; ++i) {
    auto structs = repeated_test.GetStructsEncoder();
    structs.WriteOne(i * 1);
    structs.WriteTwo(i * 2);
  }

  // clang-format off
  constexpr uint8_t expected_proto[] = {
    0x2a, 0x04, 0x08, 0x00, 0x10, 0x00, 0x2a, 0x04, 0x08,
    0x01, 0x10, 0x02, 0x2a, 0x04, 0x08, 0x02, 0x10, 0x04};
  // clang-format on

  Result result = encoder.Encode();
  ASSERT_EQ(result.status(), OkStatus());
  EXPECT_EQ(result.value().size(), sizeof(expected_proto));
  EXPECT_EQ(std::memcmp(
                result.value().data(), expected_proto, sizeof(expected_proto)),
            0);
}

TEST(Codegen, Proto2) {
  std::byte encode_buffer[64];
  NestedEncoder<1, 3> encoder(encode_buffer);

  Foo::Encoder foo(&encoder);
  foo.WriteInt(3);

  {
    constexpr std::byte data[] = {
        std::byte(0xde), std::byte(0xad), std::byte(0xbe), std::byte(0xef)};
    Bar::Encoder bar = foo.GetBarEncoder();
    bar.WriteData(data);
  }

  constexpr uint8_t expected_proto[] = {
      0x08, 0x03, 0x1a, 0x06, 0x0a, 0x04, 0xde, 0xad, 0xbe, 0xef};

  Result result = encoder.Encode();
  ASSERT_EQ(result.status(), OkStatus());
  EXPECT_EQ(result.value().size(), sizeof(expected_proto));
  EXPECT_EQ(std::memcmp(
                result.value().data(), expected_proto, sizeof(expected_proto)),
            0);
}

TEST(Codegen, Import) {
  std::byte encode_buffer[64];
  NestedEncoder<1, 3> encoder(encode_buffer);

  Period::Encoder period(&encoder);
  {
    imported::Timestamp::Encoder start = period.GetStartEncoder();
    start.WriteSeconds(1589501793);
    start.WriteNanoseconds(511613110);
  }

  {
    imported::Timestamp::Encoder end = period.GetEndEncoder();
    end.WriteSeconds(1589501841);
    end.WriteNanoseconds(490367432);
  }

  EXPECT_EQ(encoder.Encode().status(), OkStatus());
}

TEST(Codegen, NonPigweedPackage) {
  using namespace non::pigweed::package::name;
  std::byte encode_buffer[64];
  std::array<const int64_t, 2> repeated = {0, 1};
  NestedEncoder<1, 2> encoder(encode_buffer);
  Packed::Encoder packed(&encoder);
  packed.WriteRep(std::span<const int64_t>(repeated));
  packed.WritePacked("packed");

  EXPECT_EQ(encoder.Encode().status(), OkStatus());
}

}  // namespace
}  // namespace pw::protobuf
