#include <Ehternet/util.h>
#include <avr/pgmspace.h>
#include "utility/debug.h"
#include "ArduinoJetPeer.h"

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )

#define ntohl(x) htonl(x)

JetPeer::JetPeer()
  : _sock(0)
  , _msg_len(0)
  , _msg_ptr(0)
  , _req_cnt(0)
  , _state_cnt(0) {

}

void JetPeer::set_client(Client& sock) {
  _sock = &sock;
}

void JetPeer::loop(void) {
  if (_msg_len == 0) {
    while(_sock->available() && _msg_ptr < sizeof(uint32_t)) {
      _msg_buf[_msg_ptr] = _sock->read();
      ++_msg_ptr;
    }
    if (_msg_ptr == sizeof(uint32_t)) {
      uint32_t len;
      memcpy(&len, _msg_buf, sizeof(uint32_t));
      _msg_len = ntohl(len);
      _msg_ptr = 0;
    }
  }
  if (_msg_len > 0) {
    while(_sock->available() && _msg_ptr < _msg_len) {
      _msg_buf[_msg_ptr] = _sock->read();
      ++_msg_ptr;
    }
    if (_msg_ptr == _msg_len) {
      _msg_buf[_msg_ptr] = '\0';
      dispatch_message();
    }
  }
}

prog_char jet_method[] PROGMEM = "method";
prog_char jet_params[] PROGMEM = "params";
prog_char jet_id[] PROGMEM = "id";
prog_char jet_value[] PROGMEM = "value";


prog_char jet_resp_result[] PROGMEM = "{\"result\": true, \"id\":\"";
prog_char jet_resp_error[] PROGMEM = "{\"error\": {\"message\": \"Invalid params\", \"code\": -32602}, \"id\":\"";
prog_char jet_req_add[] PROGMEM = "{\"method\": \"add\", \"params\":{\"path\":\"";
prog_char jet_req_change[] PROGMEM = "{\"method\": \"change\", \"params\":{\"path\":\"";


PROGMEM const char *jet_strings[] = 	   // change "string_table" name to suit
{
  jet_req_add,
  jet_req_change,
  jet_resp_result,
  jet_resp_error,
  jet_method,
  jet_params,
  jet_id,
  jet_value
};

enum jet_string_names {
  JET_REQ_ADD,
  JET_REQ_CHANGE,
  JET_RESP_RESULT,
  JET_RESP_ERROR,
  JET_METHOD,
  JET_PARAMS,
  JET_ID,
  JET_VALUE
};


void JetPeer::dispatch_message() {
  Serial.println(_msg_buf);
  Serial.print("Free RAM 1: "); Serial.println(getFreeRam(), DEC);
  aJsonObject* msg = aJson.parse(_msg_buf);
  Serial.print("Free RAM 2: "); Serial.println(getFreeRam(), DEC);
  char* buf = _msg_buf + _msg_ptr;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_METHOD])));
  aJsonObject* method = aJson.getObjectItem(msg, buf);
  if (method) {
    String method_str(method->valuestring);
    for(int i=0; i<_state_cnt; ++i) {
      JetState& state = *_states[i];
      if (method_str.equals(state._path)) {
        strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_PARAMS])));
        aJsonObject* params = aJson.getObjectItem(msg, buf);
        aJsonObject* resp = NULL;
        strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_ID])));
        aJsonObject* id = aJson.getObjectItem(msg, buf);
        strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_VALUE])));
        aJsonObject* value = aJson.getObjectItem(params, buf);
        bool ok = state._handler(value, resp, state._context);
        if (id) {
          if (ok) {
            strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_RESP_RESULT])));
          } else {
            strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_RESP_ERROR])));
          }
          strcat(buf, id->valuestring);
          strcat(buf, "\"}");
          send(buf);
        }Serial.print("Free RAM 3: "); Serial.println(getFreeRam(), DEC);
        aJson.deleteItem(msg);
        _msg_ptr = 0;
        _msg_len = 0;
        if (ok) {
          Serial.println("auto change");
          change(state._path, value);
        }
        Serial.print("Free RAM 4: "); Serial.println(getFreeRam(), DEC);
        return;
      }
    }
  }
  aJson.deleteItem(msg);
  _msg_ptr = 0;
  _msg_len = 0;
  Serial.print("Free RAM 5: "); Serial.println(getFreeRam(), DEC);
}

void JetPeer::send(const char* msg) {
  uint32_t len = strlen(msg);
  uint32_t len_net = htonl(len);
  Serial.println(msg);
  _sock->write((const uint8_t*)&len_net, sizeof(len_net));
  _sock->write((const uint8_t*)msg, len);
}


void JetPeer::send(aJsonObject* msg) {
  aJsonStringStream stringStream(NULL, _msg_buf, 256);
  aJson.print(msg, &stringStream);
  uint32_t len = strlen(_msg_buf);
  uint32_t len_net = htonl(len);
  Serial.println(_msg_buf);
  _sock->write((const uint8_t*)&len_net, sizeof(len_net));
  _sock->write((const uint8_t*)_msg_buf, len);
}


void JetPeer::add(const char* path, aJsonObject* val) {
  char* buf = _msg_buf;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_REQ_ADD])));
  strcat(buf, path);
  strcat(buf, "\",\"value\":");
  int len_so_far = strlen(buf);
  aJsonStringStream stringStream(NULL, _msg_buf + len_so_far, 256 - len_so_far);
  aJson.print(val, &stringStream);
  strcat(buf, "}}");
  send(buf);
  aJson.deleteItem(val);
}

void JetPeer::change(const char* path, aJsonObject* val) {
  char* buf = _change_buf;
  strcpy_P(buf, (char*)pgm_read_word(&(jet_strings[JET_REQ_CHANGE])));
  strcat(buf, path);
  strcat(buf, "\",\"value\":");
  int len_so_far = strlen(buf);
  aJsonStringStream stringStream(NULL, _msg_buf + len_so_far, 128 - len_so_far);
  aJson.print(val, &stringStream);
  strcat(buf, "}}");
  send(buf);
  aJson.deleteItem(val);
}

void JetPeer::register_state(JetState& state) {
  _states[_state_cnt] = &state;
  ++_state_cnt;
}

bool default_set_handler(aJsonObject* value, aJsonObject* res, void* context) {
  Serial.print("set: "); Serial.println(value->valueint);


  return true;
}

JetState::JetState(JetPeer& peer, const char* path, aJsonObject* value)
  : _peer(peer)
  , _path(path) {
    _peer.register_state(*this);
    _peer.add(path, value);
    _handler = default_set_handler;
}

void JetState::value(aJsonObject* val) {
  _peer.change(_path, val);
}
