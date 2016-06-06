/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/types.h"
#include "base/network/url.h"
#include "base/core/str.h"

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

inline void CopyAddress(const Base::Url &url, char *hostname, char *service) {
  Base::String::strncpy(hostname, url.GetHostname(), Base::Url::kHostnameMax);
  Base::String::strncpy(service, url.GetService(), Base::Url::kServiceMax);
}

// Client to server message to request a public address assigned by NAT device.
struct BindingRequest {
  BindingRequest() {}
  BindingRequest(MessageType type, RequestId id, const Base::Url &priv)
      : type(type), id(id) {
    CopyAddress(priv, private_hostname, private_service);
  }

  MessageType type;
  RequestId id;
  char private_hostname[Base::Url::kHostnameMax];
  char private_service[Base::Url::kServiceMax];

  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

// Server to client message with a public address assigned by NAT device.
struct BindingResponse {
  BindingResponse() {}
  BindingResponse(MessageType type, RequestId id, const Base::Url &priv,
                  const Base::Url &pub)
      : type(type), id(id) {
    CopyAddress(priv, private_hostname, private_service);
    CopyAddress(pub, public_hostname, public_service);
  }

  MessageType type;
  RequestId id;
  char private_hostname[Base::Url::kHostnameMax];
  char private_service[Base::Url::kServiceMax];
  char public_hostname[Base::Url::kHostnameMax];
  char public_service[Base::Url::kServiceMax];

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
  ConnectRequest() {}
  ConnectRequest(MessageType type, RequestId id, const Base::Url &a_priv,
                 const Base::Url &a_pub, const Base::Url &b_priv,
                 const Base::Url &b_pub)
      : type(type), id(id) {
    CopyAddress(a_priv, a_private_hostname, a_private_service);
    CopyAddress(a_pub, a_public_hostname, a_public_service);
    CopyAddress(b_priv, b_private_hostname, b_private_service);
    CopyAddress(b_pub, b_public_hostname, b_public_service);
  }
  MessageType type;
  RequestId id;
  char a_private_hostname[Base::Url::kHostnameMax];
  char a_private_service[Base::Url::kServiceMax];
  char a_public_hostname[Base::Url::kHostnameMax];
  char a_public_service[Base::Url::kServiceMax];
  char b_private_hostname[Base::Url::kHostnameMax];
  char b_private_service[Base::Url::kServiceMax];
  char b_public_hostname[Base::Url::kHostnameMax];
  char b_public_service[Base::Url::kServiceMax];

  // todo(kstasik): impl
  void hton() {}
  void ntoh() {}
};

// Server to client message initlizing a connection.
struct ConnectStart {
  ConnectStart() {}
  ConnectStart(MessageType type, RequestId request_id, RequestId connect_id,
               const Base::Url &priv, const Base::Url &pub)
      : request_id(request_id), connect_id(connect_id) {
    CopyAddress(priv, private_hostname, private_service);
    CopyAddress(pub, public_hostname, public_service);
  }
  MessageType type;
  // punch request id received and passed back to the client.
  RequestId request_id;
  // connect operation id. same on both connecting clients.
  RequestId connect_id;
  char private_hostname[Base::Url::kHostnameMax];
  char private_service[Base::Url::kServiceMax];
  char public_hostname[Base::Url::kHostnameMax];
  char public_service[Base::Url::kServiceMax];

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
