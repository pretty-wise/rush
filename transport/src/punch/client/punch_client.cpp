/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "punch_client.h"

#include "../common/punch_defs.h"

#include <algorithm>

namespace Rush {
namespace Punch {

bool IsAddressValid(const Base::Url &addr) {
  return addr.GetPort() != 0 && addr.GetAddress().GetRaw() != 0;
}

bool IsValid(const PunchConfig &config) {
  if(!config.established_cb || !config.connected_cb || !config.resolved_cb ||
     !IsAddressValid(config.private_address) || !IsAddressValid(config.punch)) {
    return false;
  }
  return true;
}

PunchClient::PunchClient(const PunchConfig &config)
    : m_private_addr(config.private_address), m_socket(config.socket),
      m_generator(0) {
  m_resolveData = {config.socket, config.resolved_cb, config.data,
                   config.punch};
  m_connectData = {config.socket, config.punch, config.established_cb,
                   config.connected_cb, config.data};
}

PunchClient::~PunchClient() {}

punch_op PunchClient::ResolvePublicAddress(u32 timeout_ms) {
  BASE_DEBUG(kPunchLog, "resolving public address with timeout %dms",
             timeout_ms);

  Base::Url private_address = m_private_addr;
  RequestId req_id = GenerateId();
  ResolveOperation data;
  CreateResolveOp(&data, req_id, m_socket, private_address);

  const u32 kInstantTimeout = 0;
  if(!m_resolve.Add(req_id, data, kInstantTimeout)) {
    return 0;
  }
  return req_id;
}

punch_op PunchClient::Connect(const Base::Url &a_private,
                              const Base::Url &a_public,
                              const Base::Url &b_private,
                              const Base::Url &b_public) {
  BASE_INFO(kPunchLog, "connect %d.%d.%d%d:%d (%d.%d.%d.%d:%d) with "
                       "%d.%d.%d.%d:%d (%d.%d.%d.%d:%d)",
            PRINTF_URL(a_public), PRINTF_URL(a_private), PRINTF_URL(b_public),
            PRINTF_URL(b_private));

  RequestId req_id = GenerateId();

  ConnectRequest data = {};
  CreateConnectRequest(&data, req_id, a_private, a_public, b_private, b_public);

  const u32 kInstantTimeout = 0;
  if(!m_connect_request.Add(req_id, data, kInstantTimeout)) {
    return 0;
  }
  return req_id;
}

void PunchClient::Update(u32 delta_ms) {
  m_resolve.Timeout([&](RequestId id, ResolveOperation &data) {
    return UpdateResolveOp(id, &data, m_resolveData);
  }, delta_ms);

  m_connect_request.Timeout([&](RequestId id, ConnectRequest &data) {
    return UpdateConnectRequest(id, &data, m_connectData);
  }, delta_ms);

  m_connect_op.Timeout([&](RequestId id, ConnectOperation &data) {
    return UpdateConnectOp(id, &data, m_connectData);
  }, delta_ms);
}

void PunchClient::Read(const void *buffer, streamsize nbytes,
                       const Base::Url &from) {
  MessageType type = reinterpret_cast<MessageType>(*(MessageType *)buffer);

  if(type == MessageType::kMappedAddressResponse) {
    Msg::BindingResponse message = *(const Msg::BindingResponse *)buffer;
    message.ntoh();

    BASE_DEBUG(kPunchLog, "resolve in(kMappedAddressResponse)");

    Base::Url public_url(message.public_address, message.public_port);
    Base::Url private_url(message.private_address, message.private_port);
    bool updated =
        m_resolve.SetTimeout(message.id, [&](ResolveOperation &data) {
          OnBindingResponse(message.id, &data, private_url, public_url,
                            m_resolveData);
          return UpdateResolveOp(message.id, &data, m_resolveData);
        });
    if(!updated) {
      BASE_ERROR(kPunchLog,
                 "received binding response for unknown operation: id(%d)",
                 message.id);
    }

  } else if(type == MessageType::kMappedAddressKeepAlive) {
    BASE_INFO(kPunchLog, "in(kKeepAlive)");
  } else if(type == MessageType::kPunchThroughStart) {
    Msg::ConnectStart message = (*(const Msg::ConnectStart *)buffer);
    message.ntoh();

    Base::Url priv_addr(message.private_address, message.private_port);
    Base::Url pub_addr(message.public_address, message.public_port);

    BASE_DEBUG(kPunchLog, "connect in(kPunchThroughStart) connect(%d) id(%d)",
               message.connect_id, message.request_id);

    const u32 kInstantTimeout = 0;
    RequestId connect_id = message.connect_id;

    bool outgoing = message.request_id != 0;
    ConnectOperation data;
    CreateConnectOp(&data, connect_id, message.request_id, priv_addr, pub_addr,
                    outgoing);
    if(outgoing) {
      bool request_found = m_connect_request.Remove(message.request_id);
      if(!request_found) {
        BASE_DEBUG(kPunchLog, "received connect, but no request found. "
                              "this might be a duplicate start");
        return;
      }
      if(!m_connect_op.Add(connect_id, data, kInstantTimeout)) {
        BASE_ERROR(kPunchLog,
                   "received connect, but no more memory to continue");
        m_connectData.established_cb(kPunchFailure, message.request_id, 0, 0,
                                     m_connectData.data);
        return;
      }
      // success outgoing
      BASE_DEBUG(kPunchLog, "starting outgoing connect");
    } else {
      if(m_connect_op.Has(connect_id)) {
        BASE_DEBUG(kPunchLog, "connect(%d) already exists", connect_id);
        return;
      }
      if(!m_connect_op.Add(connect_id, data, kInstantTimeout)) {
        BASE_ERROR(kPunchLog, "received connect(%d), but out of memory",
                   connect_id);
        return;
      }
      // success incoming
      BASE_DEBUG(kPunchLog, "starting incoming connect");
    }
  } else if(type == MessageType::kPunchThroughProbeRequest) {
    const Msg::ConnectProbe &message = (*(const Msg::ConnectProbe *)buffer);

    bool updated = m_connect_op.SetTimeout(
        message.connect_id, [&](ConnectOperation &data) {
          OnProbeRequest(message.connect_id, &data, from, message.dest,
                         m_connectData);
          return UpdateConnectOp(message.connect_id, &data, m_connectData);
        });

    if(!updated) {
      BASE_DEBUG(
          kPunchLog,
          "connect in(kPunchThroughProbeRequest) probe for unknown connect(%d)",
          message.connect_id);
    }
  } else if(type == MessageType::kPunchThroughProbeResponse) {
    Msg::ConnectProbe message = (*(const Msg::ConnectProbe *)buffer);
    message.ntoh();

    bool updated = m_connect_op.SetTimeout(
        message.connect_id, [&](ConnectOperation &data) {
          OnProbeResponse(message.connect_id, &data, from, message.dest,
                          m_connectData);
          return UpdateConnectOp(message.connect_id, &data, m_connectData);
        });
    if(!updated) {
      BASE_WARN(kPunchLog,
                "in(kPunchThroughProbeResponse) for unknown connect(%d)",
                message.connect_id);
    }
  }
}

void PunchClient::Abort(punch_op id) {
  bool removed = m_resolve.Remove(id, [&](ResolveOperation &data) {
    BASE_INFO(kPunchLog, "resolve aborted %d", id);
    m_resolveData.resolved_cb(kPunchAborted, 0, 0, m_resolveData.data);
  });

  if(removed) {
    return;
  }
  removed = m_connect_request.Remove(id, [&](ConnectRequest &data) {
    BASE_INFO(kPunchLog, "punch aborted %d", id);
    m_connectData.established_cb(kPunchAborted, id, 0, 0, m_connectData.data);
  });

  if(removed) {
    return;
  }

  removed = false; // todo(kstasik): abort m_connect_op
}

punch_op PunchClient::GenerateId() {
  punch_op id = m_generator++;
  if(id <= 0) {
    m_generator = 1;
    id = 1;
  }
  return id;
}

} // namespace Punch
} // namespace Rush
