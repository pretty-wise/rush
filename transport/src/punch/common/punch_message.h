/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/types.h"

namespace Rush {
namespace Punch {

typedef s32 RequestId;

enum class MessageType : u32 {
  // messages related to NAT device assigned address discovery
  kMappedAddressRequest,
  kMappedAddressResponse,
	kMappedAddressKeepAlive,
  // messages related to NAT punch-through
  kPunchThroughRequest,
  kPunchThroughStart,
  kPunchThroughProbeRequest,
  kPunchThroughProbeResponse
};

namespace Msg {

// Client to server message to request a public address assigned by NAT device.
struct BindingRequest {
  MessageType type;
  RequestId id;
  u32 private_address;
  u16 private_port;

  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

// Server to client message with a public address assigned by NAT device.
struct BindingResponse {
  MessageType type;
  RequestId id;
  u32 private_address;
  u16 private_port;
  u32 public_address;
  u16 public_port;

  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

struct KeepAlive {
	MessageType type;
  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

// Client to server to reqeust a connect operation.
struct ConnectRequest {
  MessageType type;
  RequestId id;
  u32 a_private_address;
  u16 a_private_port;
  u32 a_public_address;
  u16 a_public_port;
  u32 b_private_address;
  u16 b_private_port;
  u32 b_public_address;
  u16 b_public_port;

  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

// Server to client message initlizing a connection.
struct ConnectStart {
  MessageType type;
  // punch request id received and passed back to the client.
  RequestId request_id;
  // connect operation id. same on both connecting clients.
  RequestId connect_id;
  u32 public_address;
  u32 private_address;
  u16 public_port;
  u16 private_port;

  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

struct ConnectProbe {
  MessageType type;
  enum Type { kPublic, kPrivate } dest;
  RequestId connect_id;

  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

} // namespace Msg
} // namespace Punch
} // namespace Rush
