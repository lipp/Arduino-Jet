#include <avr/pgmspace.h>
#include "ArduinoJetPeer.h"

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )

#define ntohl(x) htonl(x)

char string_buf[JET_MESSAGE_RAM];

char json_buf[JET_JSON_RAM];
int json_ptr = 0;

void* malloc_json(size_t len) {
    if ((len + json_ptr) > JET_JSON_RAM) {
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
  , _state_cnt(0)
  , _fetch_cnt(0) {
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
prog_char jet_path[] PROGMEM = "path";
prog_char jet_event[] PROGMEM = "event";
prog_char jet_value_cat[] PROGMEM = "\",\"value\":";


prog_char jet_resp_result[] PROGMEM = "{\"result\":true,\"id\":\"";
prog_char jet_resp_error[] PROGMEM = "{\"error\":{\"message\":\"Invalid params\",\"code\":-32602},\"id\":\"";
prog_char jet_req_add[] PROGMEM = "{\"method\":\"add\",\"params\":{\"path\":\"";
prog_char jet_req_change[] PROGMEM = "{\"method\":\"change\",\"params\":{\"path\":\"";
prog_char jet_req_fetch[] PROGMEM = "{\"method\":\"fetch\",\"params\":";
prog_char jet_req_set[] PROGMEM = "{\"method\":\"set\",\"params\":";
prog_char jet_req_call[] PROGMEM = "{\"method\":\"call\",\"params\":";


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
  jet_value_cat,
  jet_req_fetch,
  jet_path,
  jet_event,
  jet_req_set,
  jet_req_call
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
  JET_VALUE_CAT,
  JET_REQ_FETCH,
  JET_PATH,
  JET_EVENT,
  JET_REQ_SET,
  JET_REQ_CALL
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
  if (method && method->type == aJson_String) {
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
  } else if (method && method->type == aJson_Int) {
    JetFetcher& fetcher = _fetchers[method->valueint];
    strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_PARAMS])));
    aJsonObject* params = aJson.getObjectItem(msg, buf);
    strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_VALUE])));
    aJsonObject* value = aJson.getObjectItem(params, buf);
    strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_PATH])));
    aJsonObject* path = aJson.getObjectItem(params, buf);
    strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_EVENT])));
    aJsonObject* event = aJson.getObjectItem(params, buf);
    fetcher._handler(path->valuestring, event->valuestring, value, fetcher._context);
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
  aJsonStringStream stringStream(NULL, buf + len_so_far, JET_MESSAGE_RAM - len_so_far);
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

void JetPeer::fetch_request(int fetch_id, aJsonObject* fetch_expr) {
  char* buf = string_buf;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_REQ_FETCH])));
  int len_so_far = strlen(buf);
  aJson.addItemToObject(fetch_expr, "id", aJson.createItem(fetch_id));
  aJsonStringStream stringStream(NULL, buf + len_so_far, JET_MESSAGE_RAM - len_so_far);
  aJson.print(fetch_expr, &stringStream);
  strcat(buf, "}");
  send(buf);
}

void JetPeer::set(const char* path, aJsonObject* value) {
  char* buf = string_buf;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_REQ_SET])));
  int len_so_far = strlen(buf);
  aJsonObject* params = aJson.createObject();
  aJson.addItemToObject(params, "path", aJson.createItem(path));
  aJson.addItemToObject(params, "value", value);
  aJsonStringStream stringStream(NULL, buf + len_so_far, JET_MESSAGE_RAM - len_so_far);
  aJson.print(params, &stringStream);
  strcat(buf, "}");
  send(buf);
}

void JetPeer::call(const char* path, aJsonObject* args) {
  char* buf = string_buf;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_REQ_CALL])));
  int len_so_far = strlen(buf);
  aJsonObject* params = aJson.createObject();
  aJson.addItemToObject(params, "path", aJson.createItem(path));
  aJson.addItemToObject(params, "args", args);
  aJsonStringStream stringStream(NULL, buf + len_so_far, JET_MESSAGE_RAM - len_so_far);
  aJson.print(params, &stringStream);
  strcat(buf, "}");
  send(buf);
}

JetFetcher* JetPeer::fetch(const char* path, fetch_handler_t handler, void* context) {
  JetFetcher& fetcher = _fetchers[_fetch_cnt];
  fetcher._handler = handler;
  fetcher._context = context;
  aJsonObject* fetch_expr = aJson.createObject();
  aJsonObject* path_obj = aJson.createObject();
  aJson.addItemToObject(path_obj, "equals", aJson.createItem(path));
  aJson.addItemToObject(fetch_expr, "path", path_obj);
  fetch_request(_fetch_cnt, fetch_expr);
  _fetch_cnt++;
  return &fetcher;
}


bool default_set_handler(aJsonObject* value, void* context) {
  return false;
}

JetState* JetPeer::state(const char* path, aJsonObject* val, set_handler_t handler, void* context) {
  JetState& state = _states[_state_cnt];
  state._path = path;
  if (handler) {
    state._handler = handler;
    state._context = context;
  } else {
    state._handler = default_set_handler;
  }
  state._peer = this;
  add(path, val);
  _state_cnt++;
  return &state;
}

void JetState::value(aJsonObject* val) {
  _peer->change(_path, val);
}
