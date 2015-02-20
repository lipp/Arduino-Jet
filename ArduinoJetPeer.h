#ifndef ARDUINO_JET_PEER
#define ARDUINO_JET_PEER


#include <stdint.h>
#include "aJson.h"
#include "Client.h"

#ifndef JET_NUM_STATES
  #define JET_NUM_STATES 3
#endif

#ifndef JET_NUM_FETCHERS
  #define JET_NUM_FETCHERS 3
#endif

#ifndef JET_JSON_RAM
  #define JET_JSON_RAM 256
#endif

#ifndef JET_MESSAGE_RAM
  #define JET_MESSAGE_RAM 128
#endif

struct JetPeer;
typedef bool (*set_handler_t)(aJsonObject* val, void* context);
typedef void (*fetch_handler_t)(const char* path, const char* event, aJsonObject* val, void* context);

struct JetState {
  friend class JetPeer;
  void value(aJsonObject* val);
private:
  JetState(){};
  const char* _path;
  set_handler_t _handler;
  void* _context;
  JetPeer* _peer;
};

struct JetFetcher {
  friend class JetPeer;
  //void unfetch();
private:
  JetFetcher(){};
  const char* _id;
  fetch_handler_t _handler;
  void* _context;
  JetPeer* _peer;
};

struct JetPeer {
  friend class JetState;
  JetPeer();
  void init(Client& sock);
  void loop(void);
  JetState* state(const char* path, aJsonObject* val, set_handler_t handler = NULL, void *context = NULL);
  JetFetcher* fetch(const char* path, fetch_handler_t handler, void* context = NULL);
  void set(const char* path, aJsonObject* value);
  void call(const char* path, aJsonObject* args);
private:
  void value_request(const char* path, aJsonObject* val, int req_id);
  void fetch_request(int fetch_id, aJsonObject* fetch_expr);
  void add(const char* path, aJsonObject* val);
  void change(const char* path, aJsonObject* val);
  void send(aJsonObject *msg);
  void send(const char* msg);
  void dispatch_message(void);
  void register_state(JetState& state);

  Client* _sock;
  uint32_t _req_cnt;
  JetState _states[JET_NUM_STATES];
  JetFetcher _fetchers[JET_NUM_FETCHERS];
  int _fetch_cnt;
  int _state_cnt;
};

#endif
