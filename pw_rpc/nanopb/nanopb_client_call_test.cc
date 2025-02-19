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

#include "pw_rpc/nanopb_client_call.h"

#include "gtest/gtest.h"
#include "pw_rpc_nanopb_private/internal_test_utils.h"
#include "pw_rpc_private/internal_test_utils.h"
#include "pw_rpc_test_protos/test.pb.h"

namespace pw::rpc {
namespace {

constexpr uint32_t kServiceId = 16;
constexpr uint32_t kUnaryMethodId = 111;
constexpr uint32_t kServerStreamingMethodId = 112;

class FakeGeneratedServiceClient {
 public:
  static NanopbClientCall<internal::UnaryCallbacks<pw_rpc_test_TestResponse>>
  TestRpc(Channel& channel,
          const pw_rpc_test_TestRequest& request,
          Function<void(const pw_rpc_test_TestResponse&, Status)> on_response,
          Function<void(Status)> on_error = nullptr) {
    auto call = NanopbClientCall(
        &channel,
        kServiceId,
        kUnaryMethodId,
        internal::UnaryCallbacks(std::move(on_response), std::move(on_error)),
        pw_rpc_test_TestRequest_fields,
        pw_rpc_test_TestResponse_fields);
    call.SendRequest(&request);
    return call;
  }

  static NanopbClientCall<
      internal::ServerStreamingCallbacks<pw_rpc_test_TestStreamResponse>>
  TestStreamRpc(
      Channel& channel,
      const pw_rpc_test_TestRequest& request,
      Function<void(const pw_rpc_test_TestStreamResponse&)> on_response,
      Function<void(Status)> on_stream_end,
      Function<void(Status)> on_error = nullptr) {
    auto call = NanopbClientCall(
        &channel,
        kServiceId,
        kServerStreamingMethodId,
        internal::ServerStreamingCallbacks(std::move(on_response),
                                           std::move(on_stream_end),
                                           std::move(on_error)),
        pw_rpc_test_TestRequest_fields,
        pw_rpc_test_TestStreamResponse_fields);
    call.SendRequest(&request);
    return call;
  }
};

TEST(NanopbClientCall, Unary_SendsRequestPacket) {
  ClientContextForTest context;

  auto call = FakeGeneratedServiceClient::TestRpc(
      context.channel(), {.integer = 123, .status_code = 0}, nullptr);

  EXPECT_EQ(context.output().packet_count(), 1u);
  auto packet = context.output().sent_packet();
  EXPECT_EQ(packet.channel_id(), context.channel().id());
  EXPECT_EQ(packet.service_id(), kServiceId);
  EXPECT_EQ(packet.method_id(), kUnaryMethodId);

  PW_DECODE_PB(pw_rpc_test_TestRequest, sent_proto, packet.payload());
  EXPECT_EQ(sent_proto.integer, 123);
}

class UnaryClientCall : public ::testing::Test {
 protected:
  Status last_status_ = Status::Unknown();
  Status last_error_ = Status::Unknown();
  int responses_received_ = 0;
  int last_response_value_ = 0;
};

TEST_F(UnaryClientCall, InvokesCallbackOnValidResponse) {
  ClientContextForTest context;

  auto call = FakeGeneratedServiceClient::TestRpc(
      context.channel(),
      {.integer = 123, .status_code = 0},
      [this](const pw_rpc_test_TestResponse& response, Status status) {
        ++responses_received_;
        last_status_ = status;
        last_response_value_ = response.value;
      });

  PW_ENCODE_PB(pw_rpc_test_TestResponse, response, .value = 42);
  context.SendResponse(OkStatus(), response);

  ASSERT_EQ(responses_received_, 1);
  EXPECT_EQ(last_status_, OkStatus());
  EXPECT_EQ(last_response_value_, 42);
}

TEST_F(UnaryClientCall, DoesNothingOnNullCallback) {
  ClientContextForTest context;

  auto call = FakeGeneratedServiceClient::TestRpc(
      context.channel(), {.integer = 123, .status_code = 0}, nullptr);

  PW_ENCODE_PB(pw_rpc_test_TestResponse, response, .value = 42);
  context.SendResponse(OkStatus(), response);

  ASSERT_EQ(responses_received_, 0);
}

TEST_F(UnaryClientCall, InvokesErrorCallbackOnInvalidResponse) {
  ClientContextForTest context;

  auto call = FakeGeneratedServiceClient::TestRpc(
      context.channel(),
      {.integer = 123, .status_code = 0},
      [this](const pw_rpc_test_TestResponse& response, Status status) {
        ++responses_received_;
        last_status_ = status;
        last_response_value_ = response.value;
      },
      [this](Status status) { last_error_ = status; });

  constexpr std::byte bad_payload[]{
      std::byte{0xab}, std::byte{0xcd}, std::byte{0xef}};
  context.SendResponse(OkStatus(), bad_payload);

  EXPECT_EQ(responses_received_, 0);
  EXPECT_EQ(last_error_, Status::DataLoss());
}

TEST_F(UnaryClientCall, InvokesErrorCallbackOnServerError) {
  ClientContextForTest context;

  auto call = FakeGeneratedServiceClient::TestRpc(
      context.channel(),
      {.integer = 123, .status_code = 0},
      [this](const pw_rpc_test_TestResponse& response, Status status) {
        ++responses_received_;
        last_status_ = status;
        last_response_value_ = response.value;
      },
      [this](Status status) { last_error_ = status; });

  context.SendPacket(internal::PacketType::SERVER_ERROR, Status::NotFound());

  EXPECT_EQ(responses_received_, 0);
  EXPECT_EQ(last_error_, Status::NotFound());
}

TEST_F(UnaryClientCall, DoesNothingOnErrorWithoutCallback) {
  ClientContextForTest context;

  auto call = FakeGeneratedServiceClient::TestRpc(
      context.channel(),
      {.integer = 123, .status_code = 0},
      [this](const pw_rpc_test_TestResponse& response, Status status) {
        ++responses_received_;
        last_status_ = status;
        last_response_value_ = response.value;
      });

  constexpr std::byte bad_payload[]{
      std::byte{0xab}, std::byte{0xcd}, std::byte{0xef}};
  context.SendResponse(OkStatus(), bad_payload);

  EXPECT_EQ(responses_received_, 0);
}

TEST_F(UnaryClientCall, OnlyReceivesOneResponse) {
  ClientContextForTest context;

  auto call = FakeGeneratedServiceClient::TestRpc(
      context.channel(),
      {.integer = 123, .status_code = 0},
      [this](const pw_rpc_test_TestResponse& response, Status status) {
        ++responses_received_;
        last_status_ = status;
        last_response_value_ = response.value;
      });

  PW_ENCODE_PB(pw_rpc_test_TestResponse, r1, .value = 42);
  context.SendResponse(Status::Unimplemented(), r1);
  PW_ENCODE_PB(pw_rpc_test_TestResponse, r2, .value = 44);
  context.SendResponse(Status::OutOfRange(), r2);
  PW_ENCODE_PB(pw_rpc_test_TestResponse, r3, .value = 46);
  context.SendResponse(Status::Internal(), r3);

  EXPECT_EQ(responses_received_, 1);
  EXPECT_EQ(last_status_, Status::Unimplemented());
  EXPECT_EQ(last_response_value_, 42);
}

class ServerStreamingClientCall : public ::testing::Test {
 protected:
  bool active_ = true;
  Status stream_status_ = Status::Unknown();
  Status rpc_error_ = Status::Unknown();
  int responses_received_ = 0;
  int last_response_number_ = 0;
};

TEST_F(ServerStreamingClientCall, SendsRequestPacket) {
  ClientContextForTest<128, 128, 99, kServiceId, kServerStreamingMethodId>
      context;

  auto call = FakeGeneratedServiceClient::TestStreamRpc(
      context.channel(), {.integer = 71, .status_code = 0}, nullptr, nullptr);

  EXPECT_EQ(context.output().packet_count(), 1u);
  auto packet = context.output().sent_packet();
  EXPECT_EQ(packet.channel_id(), context.channel().id());
  EXPECT_EQ(packet.service_id(), kServiceId);
  EXPECT_EQ(packet.method_id(), kServerStreamingMethodId);

  PW_DECODE_PB(pw_rpc_test_TestRequest, sent_proto, packet.payload());
  EXPECT_EQ(sent_proto.integer, 71);
}

TEST_F(ServerStreamingClientCall, InvokesCallbackOnValidResponse) {
  ClientContextForTest<128, 128, 99, kServiceId, kServerStreamingMethodId>
      context;

  auto call = FakeGeneratedServiceClient::TestStreamRpc(
      context.channel(),
      {.integer = 71, .status_code = 0},
      [this](const pw_rpc_test_TestStreamResponse& response) {
        ++responses_received_;
        last_response_number_ = response.number;
      },
      [this](Status status) {
        active_ = false;
        stream_status_ = status;
      });

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r1, .chunk = {}, .number = 11u);
  context.SendResponse(OkStatus(), r1);
  EXPECT_TRUE(active_);
  EXPECT_EQ(responses_received_, 1);
  EXPECT_EQ(last_response_number_, 11);

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r2, .chunk = {}, .number = 22u);
  context.SendResponse(OkStatus(), r2);
  EXPECT_TRUE(active_);
  EXPECT_EQ(responses_received_, 2);
  EXPECT_EQ(last_response_number_, 22);

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r3, .chunk = {}, .number = 33u);
  context.SendResponse(OkStatus(), r3);
  EXPECT_TRUE(active_);
  EXPECT_EQ(responses_received_, 3);
  EXPECT_EQ(last_response_number_, 33);
}

TEST_F(ServerStreamingClientCall, InvokesStreamEndOnFinish) {
  ClientContextForTest<128, 128, 99, kServiceId, kServerStreamingMethodId>
      context;

  auto call = FakeGeneratedServiceClient::TestStreamRpc(
      context.channel(),
      {.integer = 71, .status_code = 0},
      [this](const pw_rpc_test_TestStreamResponse& response) {
        ++responses_received_;
        last_response_number_ = response.number;
      },
      [this](Status status) {
        active_ = false;
        stream_status_ = status;
      });

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r1, .chunk = {}, .number = 11u);
  context.SendResponse(OkStatus(), r1);
  EXPECT_TRUE(active_);

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r2, .chunk = {}, .number = 22u);
  context.SendResponse(OkStatus(), r2);
  EXPECT_TRUE(active_);

  // Close the stream.
  context.SendPacket(internal::PacketType::SERVER_STREAM_END,
                     Status::NotFound());

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r3, .chunk = {}, .number = 33u);
  context.SendResponse(OkStatus(), r3);
  EXPECT_FALSE(active_);

  EXPECT_EQ(responses_received_, 2);
}

TEST_F(ServerStreamingClientCall, InvokesErrorCallbackOnInvalidResponses) {
  ClientContextForTest<128, 128, 99, kServiceId, kServerStreamingMethodId>
      context;

  auto call = FakeGeneratedServiceClient::TestStreamRpc(
      context.channel(),
      {.integer = 71, .status_code = 0},
      [this](const pw_rpc_test_TestStreamResponse& response) {
        ++responses_received_;
        last_response_number_ = response.number;
      },
      nullptr,
      [this](Status error) { rpc_error_ = error; });

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r1, .chunk = {}, .number = 11u);
  context.SendResponse(OkStatus(), r1);
  EXPECT_TRUE(active_);
  EXPECT_EQ(responses_received_, 1);
  EXPECT_EQ(last_response_number_, 11);

  constexpr std::byte bad_payload[]{
      std::byte{0xab}, std::byte{0xcd}, std::byte{0xef}};
  context.SendResponse(OkStatus(), bad_payload);
  EXPECT_EQ(responses_received_, 1);
  EXPECT_EQ(rpc_error_, Status::DataLoss());

  PW_ENCODE_PB(pw_rpc_test_TestStreamResponse, r2, .chunk = {}, .number = 22u);
  context.SendResponse(OkStatus(), r2);
  EXPECT_TRUE(active_);
  EXPECT_EQ(responses_received_, 2);
  EXPECT_EQ(last_response_number_, 22);

  context.SendPacket(internal::PacketType::SERVER_ERROR, Status::NotFound());
  EXPECT_EQ(responses_received_, 2);
  EXPECT_EQ(rpc_error_, Status::NotFound());
}

}  // namespace
}  // namespace pw::rpc
