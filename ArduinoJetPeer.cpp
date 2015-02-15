#include <Ehternet/util.h>
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

void JetPeer::dispatch_message() {
  Serial.println(_msg_buf);
  aJsonObject* msg = aJson.parse(_msg_buf);
  aJsonObject* method = aJson.getObjectItem(msg, "method");
  if (method) {
    String method_str(method->valuestring);
    for(int i=0; i<_state_cnt; ++i) {
      if (method_str.equals(_states[i]->_path)) {
        aJsonObject* params = aJson.getObjectItem(msg, "params");
        _states[i]->_handler(aJson.getObjectItem(params, "value")->valueint, _states[i]->_context);
      }
    }
  }
  _msg_ptr = 0;
  _msg_len = 0;
}

void JetPeer::send(aJsonObject* msg) {
  aJsonStringStream stringStream(NULL, _msg_buf, 256);
  aJson.print(msg, &stringStream);
  uint32_t len = strlen(_msg_buf);
  uint32_t len_net = htonl(len);
  Serial.println(len);
  Serial.println(len_net);
  uint8_t *bytes = (uint8_t*)&len_net;
  Serial.println(bytes[0]);
  Serial.println(bytes[1]);
  Serial.println(bytes[2]);
  Serial.println(bytes[3]);
  _sock->write((const uint8_t*)&len_net, sizeof(len_net));
  _sock->write((const uint8_t*)_msg_buf, len);
}

void JetPeer::add(const char* path, aJsonObject* val) {
  aJsonObject *msg = aJson.createObject();
  aJsonObject *params = aJson.createObject();
  aJson.addStringToObject(msg, "method", "add");
  aJson.addStringToObject(params, "path", path);
  if (val) {
    aJson.addItemToObject(params, "value", val);
  }
  aJson.addItemToObject(msg, "params", params);
  send(msg);
  aJson.deleteItem(msg);
}

void JetPeer::change(const char* path, aJsonObject* val) {
  aJsonObject *msg = aJson.createObject();
  aJsonObject *params = aJson.createObject();
  aJson.addStringToObject(msg, "method", "change");
  aJson.addStringToObject(params, "path", path);
  if (val) {
    aJson.addItemToObject(params, "value", val);
  }
  aJson.addItemToObject(msg, "params", params);
  send(msg);
  aJson.deleteItem(msg);
}

void JetPeer::register_state(JetState& state) {
  _states[_state_cnt] = &state;
  ++_state_cnt;
}

int default_set_handler(int val, void* context) {
  Serial.print("set: "); Serial.println(val);
}

JetState::JetState(JetPeer& peer, const char* path, int val)
  : _peer(peer)
  , _path(path) {
    _peer.register_state(*this);
    _peer.add(path, aJson.createItem(val));
    _handler = default_set_handler;
}

void JetState::value(int val) {
  _peer.change(_path, aJson.createItem(val));
}
