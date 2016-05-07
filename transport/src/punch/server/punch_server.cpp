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

void Send(Base::Socket::Handle socket, Base::Url &to, const void *data,
          u16 nbytes) {
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
  // common setup
  Rush::Punch::Msg::ConnectStart origin;
  Rush::Punch::Msg::ConnectStart target;
  origin.type = Rush::Punch::MessageType::kPunchThroughStart;
  target.type = Rush::Punch::MessageType::kPunchThroughStart;
  origin.request_id = op->request_id; // pass request id back to the client.
  target.request_id = 0;              // target doesn't get the request id.
  origin.connect_id = op->connect_id;
  target.connect_id = op->connect_id;
  // message to origin
  origin.public_address = op->b_public.GetAddress().GetRaw();
  origin.public_port = op->b_public.GetPort();
  origin.private_address = op->b_private.GetAddress().GetRaw();
  origin.private_port = op->b_private.GetPort();
  Send(socket, op->a_public, reinterpret_cast<const void *>(&origin),
       sizeof(origin));
  // message to target
  target.public_address = op->a_public.GetAddress().GetRaw();
  target.public_port = op->a_public.GetPort();
  target.private_address = op->a_private.GetAddress().GetRaw();
  target.private_port = op->a_private.GetPort();
  Send(socket, op->b_public, reinterpret_cast<const void *>(&target),
       sizeof(target));
  BASE_INFO(kPunchLog,
            "out(kPunchThroughStart) connect(%d) to %d.%d.%d.%d:%d. reach "
            "pub(%d.%d.%d.%d:%d) prv(%d.%d.%d.%d:%d)",
            op->connect_id, PRINTF_URL(op->b_public), PRINTF_URL(op->a_public),
            PRINTF_URL(op->a_private));
  BASE_INFO(kPunchLog,
            "out(kPunchThroughStart) connect(%d) to %d.%d.%d.%d:%d. reach "
            "pub(%d.%d.%d.%d:%d) prv(%d.%d.%d.%d:%d)",
            op->connect_id, PRINTF_URL(op->a_public), PRINTF_URL(op->b_public),
            PRINTF_URL(op->b_private));
  op->punch_resend = kPunchCommandRetryMs;
}

} // unnamed namespace

PunchServer::PunchServer() : m_generator(0) {}

PunchServer::~PunchServer() { Base::Socket::Close(m_socket); }

bool PunchServer::Open(const Base::Url &url, u16 *port) {
  m_socket = Base::Socket::Udp::Open(url.GetAddress().GetRaw(), port);
  if(m_socket == Base::Socket::InvalidHandle) {
    BASE_ERROR(kPunchLog, "problem opening socket");
    return false;
  }
  m_url = Base::Url(url.GetAddress().GetRaw(), *port);
  return true;
}

void PunchServer::Update(u32 dt) {
  Base::Url from;
  const int nbytes = 1024;
  char buffer[nbytes];

  int error = 0;
  streamsize read = Base::Socket::Udp::Recv(
      m_socket, &from, static_cast<void *>(buffer), nbytes, &error);
  if(read > 0) {
    BASE_INFO(kPunchLog, "read %dbytes from %d.%d.%d.%d:%d", read,
              PRINTF_URL(from));
    Rush::Punch::MessageType type = reinterpret_cast<Rush::Punch::MessageType>(
        *(Rush::Punch::MessageType *)buffer);
    if(type == Rush::Punch::MessageType::kMappedAddressRequest) {
      Rush::Punch::Msg::BindingRequest message =
          *(const Rush::Punch::Msg::BindingRequest *)buffer;
      message.ntoh();
      Base::Url private_url(message.private_address, message.private_port);
      BASE_INFO(
          kPunchLog,
          "public address request id(%d): %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d",
          message.id, PRINTF_URL(private_url), PRINTF_URL(from));
      Rush::Punch::Msg::BindingResponse response = {
          Rush::Punch::MessageType::kMappedAddressResponse,
          message.id,
          message.private_address,
          message.private_port,
          from.GetAddress().GetRaw(),
          from.GetPort()};
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
        Base::Url a_private(message.a_private_address, message.a_private_port);
        Base::Url a_public(message.a_public_address, message.a_public_port);
        Base::Url b_private(message.b_private_address, message.b_private_port);
        Base::Url b_public(message.b_public_address, message.b_public_port);
        BASE_INFO(kPunchLog, "punch %d.%d.%d.%d:%d (%d.%d.%d.%d:%d) <-> "
                             "%d.%d.%d.%d:%d (%d.%d.%d.%d:%d)",
                  PRINTF_URL(a_public), PRINTF_URL(a_private),
                  PRINTF_URL(b_public), PRINTF_URL(b_private));
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
