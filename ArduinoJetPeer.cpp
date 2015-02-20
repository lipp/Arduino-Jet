#include <Ehternet/util.h>
#include <avr/pgmspace.h>
#include "utility/debug.h"
#include "ArduinoJetPeer.h"

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )

#define ntohl(x) htonl(x)

#define JET_RAM 256
#define JET_STRING_RAM 128

char string_buf[JET_STRING_RAM];

char json_buf[JET_RAM];
int json_ptr = 0;

void* malloc_json(size_t len) {
    if ((len + json_ptr) > JET_RAM) {
      Serial.println(F("jet out of mem"));
      return NULL;
    }
    void* ptr = (void*) (json_buf + json_ptr);
    json_ptr += len;
    return ptr;
}

void free_json(void* ptr) {

}

JetPeer::JetPeer()
  : _sock(0)
  , _req_cnt(0)
  , _state_cnt(0) {
    aJson.setMemFuncs(malloc_json, free_json);
}

void JetPeer::init(Client& sock) {
  _sock = &sock;
}

void JetPeer::loop(void) {
  json_ptr = 0;
  if (_sock->available()) {
    _sock->read();
    while (!_sock->available());
    _sock->read();
    while (!_sock->available());
    _sock->read();
    while (!_sock->available());
    _sock->read();
    dispatch_message();
    json_ptr = 0;
  }
}

prog_char jet_method[] PROGMEM = "method";
prog_char jet_params[] PROGMEM = "params";
prog_char jet_id[] PROGMEM = "id";
prog_char jet_value[] PROGMEM = "value";
prog_char jet_value_cat[] PROGMEM = "\",\"value\":";


prog_char jet_resp_result[] PROGMEM = "{\"result\":true,\"id\":\"";
prog_char jet_resp_error[] PROGMEM = "{\"error\":{\"message\":\"Invalid params\",\"code\":-32602},\"id\":\"";
prog_char jet_req_add[] PROGMEM = "{\"method\":\"add\",\"params\":{\"path\":\"";
prog_char jet_req_change[] PROGMEM = "{\"method\":\"change\",\"params\":{\"path\":\"";


PROGMEM const char *jet_strings[] = 	   // change "string_table" name to suit
{
  jet_req_add,
  jet_req_change,
  jet_resp_result,
  jet_resp_error,
  jet_method,
  jet_params,
  jet_id,
  jet_value,
  jet_value_cat
};

enum jet_string_names {
  JET_REQ_ADD,
  JET_REQ_CHANGE,
  JET_RESP_RESULT,
  JET_RESP_ERROR,
  JET_METHOD,
  JET_PARAMS,
  JET_ID,
  JET_VALUE,
  JET_VALUE_CAT
};

void JetPeer::dispatch_message() {
  aJsonStream input(_sock);
  // reset json_buf, aJson.parse will create dyn objects ->malloc
  json_ptr = 0;
  aJsonObject* msg = aJson.parse(&input);
  if (msg == NULL) {
    Serial.println(F("Parse failed"));
    return;
  }
  char* buf = (char*)string_buf;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_METHOD])));
  aJsonObject* method = aJson.getObjectItem(msg, buf);
  if (method) {
    String method_str(method->valuestring);
    for(int i=0; i<_state_cnt; ++i) {
      JetState& state = _states[i];
      if (method_str.equals(state._path)) {
        strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_PARAMS])));
        aJsonObject* params = aJson.getObjectItem(msg, buf);
        strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_ID])));
        aJsonObject* id = aJson.getObjectItem(msg, buf);
        strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_VALUE])));
        aJsonObject* value = aJson.getObjectItem(params, buf);
        bool ok = state._handler(value, state._context);
        if (id) {
          if (ok) {
            strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_RESP_RESULT])));
          } else {
            strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_RESP_ERROR])));
          }
          strcat(buf, id->valuestring);
          strcat(buf, "\"}");
          send(buf);
        }
        if (ok) {
          change(state._path, value);
        }
        return;
      }
    }
  }
}

void JetPeer::send(const char* msg) {
  uint32_t len = strlen(msg);
  uint32_t len_net = htonl(len);
  //Serial.println(msg);
  _sock->write((const uint8_t*)&len_net, sizeof(len_net));
  _sock->write((const uint8_t*)msg, len);
}

void JetPeer::value_request(const char* path, aJsonObject* val, int req_id) {
  char* buf = string_buf;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[req_id])));
  strcat(buf, path);
  strcat_P(buf, (char*)pgm_read_word(&(jet_strings[JET_VALUE_CAT])));
  int len_so_far = strlen(buf);
  aJsonStringStream stringStream(NULL, buf + len_so_far, JET_STRING_RAM - len_so_far);
  aJson.print(val, &stringStream);
  strcat(buf, "}}");
  send(buf);
}

void JetPeer::add(const char* path, aJsonObject* val) {
  value_request(path, val, JET_REQ_ADD);
}

void JetPeer::change(const char* path, aJsonObject* val) {
  value_request(path, val, JET_REQ_CHANGE);
}

bool default_set_handler(aJsonObject* value, void* context) {
  return false;
}

JetState* JetPeer::state(const char* path, aJsonObject* val) {
  JetState& state = _states[_state_cnt];
  state._path = path;
  state._handler = default_set_handler;
  state._peer = this;
  add(path, val);
  _state_cnt++;
  return &state;
}

void JetState::value(aJsonObject* val) {
  _peer->change(_path, val);
}

void JetState::set_handler(set_handler_t handler, void* context) {
  _handler = handler;
  _context = context;
}
