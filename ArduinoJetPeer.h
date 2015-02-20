#ifndef ARDUINO_JET_PEER
#define ARDUINO_JET_PEER


#include <stdint.h>
#include "aJson.h"
#include "Client.h"

#define JET_NUM_STATES 3

struct JetPeer;
typedef bool (*set_handler_t)(aJsonObject* val, void* context);
typedef void (*fetch_handler_t)(const char* path, const char* event, aJsonObject* val);

struct JetState {
  friend class JetPeer;
  void value(aJsonObject* val);
  void set_handler(set_handler_t, void* context = NULL);
private:
  JetState(){};
  const char* _path;
  set_handler_t _handler;
  void* _context;
  JetPeer* _peer;
};

struct JetPeer {
  friend class JetState;
  JetPeer();
  void init(Client& sock);
  void loop(void);
  JetState* state(const char* path, aJsonObject* val);
private:
  void value_request(const char* path, aJsonObject* val, int req_id);
  void add(const char* path, aJsonObject* val);
  void change(const char* path, aJsonObject* val);
  void send(aJsonObject *msg);
  void send(const char* msg);
  void dispatch_message(void);
  void register_state(JetState& state);

  Client* _sock;
  uint32_t _req_cnt;
  JetState _states[3];

  int _state_cnt;
};

#endif
