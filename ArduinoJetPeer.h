#ifndef ARDUINO_JET_PEER
#define ARDUINO_JET_PEER


#include <stdint.h>
#include "aJson.h"
#include "Client.h"

struct JetPeer;
typedef bool (*set_handler_t)(aJsonObject* val, aJsonObject* resp, void* context);

struct JetState {
  friend class JetPeer;
  JetState(JetPeer& peer, const char* path, aJsonObject* val);
  void value(aJsonObject* val);
  void set_handler(set_handler_t);
private:
  const char* _path;
  set_handler_t _handler;
  void* _context;
  JetPeer& _peer;
};

struct JetPeer {
  friend class JetState;
  JetPeer();
  void set_client(Client& sock);
  void loop(void);
private:
  void add(const char* path, aJsonObject* val);
  void change(const char* path, aJsonObject* val);
  void send(aJsonObject *msg);
  void send(const char* msg);
  void dispatch_message(void);
  void register_state(JetState& state);
  Client* _sock;
  uint32_t _msg_len;
  int _msg_ptr;
  char _msg_buf[256];
  char _change_buf[128];
  uint32_t _req_cnt;
  JetState* _states[3];
  int _state_cnt;
};

#endif
