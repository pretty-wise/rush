/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "punch_server.h"
#include "base/core/log.h"
#include "base/core/assert.h"

#include <cstring> // memcpy
#include <algorithm>
#include "../../packet.h"

namespace Rush {
namespace Punch {

Base::LogChannel kPunchLog("punch_server");

namespace {

void Send(Base::Socket::Handle socket, Base::Socket::Address &to,
          const void *data, u16 nbytes) {
  const u16 kMaxPacketSize = 1024;
  char packet[kMaxPacketSize];
  u16 packet_size = sizeof(Rush::PacketHeader) + nbytes;
  BASE_ASSERT(sizeof(Rush::PacketHeader) + nbytes < kMaxPacketSize,
              "packet capacity exceeded %d/%d", packet_size, kMaxPacketSize);
  Rush::PacketHeader header = {
      Rush::kPunchProtocol,
  };
  memcpy(packet, static_cast<const void *>(&header),
         sizeof(Rush::PacketHeader));
  memcpy(packet + sizeof(Rush::PacketHeader), data, nbytes);
  int error;
  Base::Socket::Udp::Send(socket, to, packet, packet_size, &error);
}

void UpdatePunch(struct PunchOperation *op, u32 dt,
                 Base::Socket::Handle socket) {
  op->punch_resend -= dt;
  if(op->punch_resend >= 0) {
    return;
  }
  // kExternal not supported yet
  BASE_ASSERT(op->type == PunchOperation::kRequest);

  // message to origin
  Rush::Punch::Msg::ConnectStart origin(
      Rush::Punch::MessageType::kPunchThroughStart,
      op->request_id, // pass request id back to the client.
      op->connect_id, op->b_public, op->b_private);

  Send(socket, op->a_public, reinterpret_cast<const void *>(&origin),
       sizeof(origin));

  // message to target
  Rush::Punch::Msg::ConnectStart target(
      Rush::Punch::MessageType::kPunchThroughStart,
      0, // target doesn't get the request id.
      op->connect_id, op->a_public, op->a_private);

  Send(socket, op->b_public, reinterpret_cast<const void *>(&target),
       sizeof(target));

  BASE_INFO(kPunchLog, "out(kPunchThroughStart) connect(%d) to %s. reach "
                       "pub(%s) prv(%s)",
            op->connect_id, Base::Socket::Print(op->b_public),
            Base::Socket::Print(op->a_public),
            Base::Socket::Print(op->a_private));
  BASE_INFO(kPunchLog, "out(kPunchThroughStart) connect(%d) to %s. reach "
                       "pub(%s) prv(%s)",
            op->connect_id, Base::Socket::Print(op->a_public),
            Base::Socket::Print(op->b_public),
            Base::Socket::Print(op->b_private));
  op->punch_resend = kPunchCommandRetryMs;
}

} // unnamed namespace

PunchServer::PunchServer() : m_generator(0) {}

PunchServer::~PunchServer() { Base::Socket::Close(m_socket); }

bool PunchServer::Open(const Base::Url &url, u16 *port) {
  m_socket = Base::Socket::Udp::Open(url, port);
  if(m_socket == Base::Socket::InvalidHandle) {
    BASE_ERROR(kPunchLog, "problem opening socket");
    return false;
  }
  m_url = Base::Url(url.GetHostname(), *port);
  return true;
}

void PunchServer::Update(u32 dt) {
  Base::Socket::Address from;
  const int nbytes = 1024;
  char buffer[nbytes];

  int error = 0;
  streamsize read = Base::Socket::Udp::Recv(
      m_socket, &from, static_cast<void *>(buffer), nbytes, &error);
  if(read > 0) {
    BASE_INFO(kPunchLog, "read %dbytes from %s", read,
              Base::Socket::Print(from));
    Rush::Punch::MessageType type = reinterpret_cast<Rush::Punch::MessageType>(
        *(Rush::Punch::MessageType *)buffer);
    if(type == Rush::Punch::MessageType::kMappedAddressRequest) {
      Rush::Punch::Msg::BindingRequest message =
          *(const Rush::Punch::Msg::BindingRequest *)buffer;
      message.ntoh();
      Base::Url private_url(message.private_hostname, message.private_service);
      BASE_INFO(kPunchLog, "public address request id(%d): %s:%s -> %s",
                message.id, PRINTF_URL(private_url), Base::Socket::Print(from));
      Rush::Punch::Msg::BindingResponse response(
          Rush::Punch::MessageType::kMappedAddressResponse, message.id,
          private_url, from);
      // todo(kstasik): hton
      Send(m_socket, from, reinterpret_cast<const void *>(&response),
           sizeof(response));
    } else if(type == Rush::Punch::MessageType::kPunchThroughStart) {
      Rush::Punch::Msg::ConnectRequest message =
          *(const Rush::Punch::Msg::ConnectRequest *)buffer;
      message.ntoh();
      BASE_INFO(kPunchLog, "connect request received id(%d)", message.id);
      // todo(kstasik): change this shitty code of dropping consecutive
      // requests.
      bool op_already_running = false;
      std::for_each(m_punch_ops.begin(), m_punch_ops.end(),
                    [&](PunchOperation &op) {
                      if(op.request_id == message.id) {
                        op_already_running = true;
                      }
                    });
      if(op_already_running) {
        BASE_INFO(kPunchLog, "punch operation already running. request_id: %d",
                  message.id);
      } else {
        Base::Socket::Address a_private, b_private, a_public, b_public;
        Base::Socket::Address::CreateUDP(message.a_private_hostname,
                                         message.a_private_service, &a_private);
        Base::Socket::Address::CreateUDP(message.a_public_hostname,
                                         message.a_public_service, &a_public);
        Base::Socket::Address::CreateUDP(message.b_private_hostname,
                                         message.b_private_service, &b_private);
        Base::Socket::Address::CreateUDP(message.b_public_hostname,
                                         message.b_public_service, &b_public);
        BASE_INFO(kPunchLog, "punch %s (%s) <-> "
                             "%s (%s)",
                  Base::Socket::Print(a_public), Base::Socket::Print(a_private),
                  Base::Socket::Print(b_public),
                  Base::Socket::Print(b_private));
        PunchOperation data;
        data.type = PunchOperation::kRequest;
        data.a_public = a_public;
        data.a_private = a_private;
        data.b_public = b_public;
        data.b_private = b_private;
        data.punch_resend = -1;
        data.timeout = kPunchCommandTimeoutMs;
        data.request_id =
            message.id; // store request_id is sent back to client.
        data.connect_id = ++m_generator; // generate a new connect operation id.
        m_punch_ops.push_back(data);
        // todo(kstasik): hton
      }
    } else if(type == Rush::Punch::MessageType::kMappedAddressKeepAlive) {
      Rush::Punch::Msg::KeepAlive keep_alive = {
          Rush::Punch::MessageType::kMappedAddressKeepAlive};
      keep_alive.hton();
      Send(m_socket, from, reinterpret_cast<const void *>(&keep_alive),
           sizeof(keep_alive));
    }
  }

  UpdatePunchOperations(dt);
}
void PunchServer::UpdatePunchOperations(u32 dt) {
  for(auto it = m_punch_ops.begin(); it != m_punch_ops.end();) {
    it->timeout -= dt;
    if(it->timeout < 0) {
      it = m_punch_ops.erase(it);
    } else {
      ++it;
    }
  }
  std::for_each(m_punch_ops.begin(), m_punch_ops.end(),
                [&](PunchOperation &op) { UpdatePunch(&op, dt, m_socket); });
}

} // namespace Punch
} // namespace Rush
