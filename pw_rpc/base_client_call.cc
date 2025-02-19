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

#include "pw_rpc/internal/base_client_call.h"

#include "pw_rpc/client.h"

namespace pw::rpc::internal {

BaseClientCall& BaseClientCall::operator=(BaseClientCall&& other) {
  // If the current client call is active, it must be unregistered from the
  // client as it will no longer be alive after assignment.
  Unregister();

  active_ = other.active_;

  if (other.active()) {
    // If the call being assigned is active, replace it in the client's list
    // with a reference to the current object.
    other.Unregister();
    other.channel_->client()->RegisterCall(*this);
  }

  channel_ = other.channel_;
  service_id_ = other.service_id_;
  method_id_ = other.method_id_;
  request_ = std::move(other.request_);
  handler_ = other.handler_;

  return *this;
}

void BaseClientCall::Cancel() {
  if (active()) {
    channel_->Send(NewPacket(PacketType::CANCEL));
  }
}

std::span<std::byte> BaseClientCall::AcquirePayloadBuffer() {
  if (!active()) {
    return {};
  }

  request_ = channel_->AcquireBuffer();
  return request_.payload(NewPacket(PacketType::REQUEST));
}

Status BaseClientCall::ReleasePayloadBuffer(
    std::span<const std::byte> payload) {
  if (!active()) {
    return Status::FailedPrecondition();
  }

  return channel_->Send(request_, NewPacket(PacketType::REQUEST, payload));
}

Packet BaseClientCall::NewPacket(PacketType type,
                                 std::span<const std::byte> payload) const {
  return Packet(type, channel_->id(), service_id_, method_id_, payload);
}

void BaseClientCall::Register() { channel_->client()->RegisterCall(*this); }

void BaseClientCall::Unregister() {
  if (active()) {
    channel_->client()->RemoveCall(*this);
    active_ = false;
  }
}

}  // namespace pw::rpc::internal
