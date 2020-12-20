#include "log.h"
#include "net.h"
#include "util.h"

int mg_vprintf(struct mg_connection *c, const char *fmt, va_list ap) {
  char mem[256], *buf = mem;
  int len = mg_vasprintf(&buf, sizeof(mem), fmt, ap);
  len = mg_send(c, buf, len);
  if (buf != mem) free(buf);
  return len;
}

int mg_printf(struct mg_connection *c, const char *fmt, ...) {
  int len = 0;
  va_list ap;
  va_start(ap, fmt);
  len = mg_vprintf(c, fmt, ap);
  va_end(ap);
  return len;
}

char *mg_straddr(struct mg_connection *c, char *buf, size_t len) {
  unsigned char *p = (unsigned char *) &c->peer.ip;
  snprintf(buf, len, "%d.%d.%d.%d:%hu", p[0], p[1], p[2], p[3],
           mg_ntohs(c->peer.port));
  return buf;
}

char *mg_ntoa(const struct mg_addr *addr, char *buf, size_t len) {
  uint8_t p[4];
  memcpy(p, &addr->ip, sizeof(p));
  snprintf(buf, len, "%hhu.%hhu.%hhu.%hhu", p[0], p[1], p[2], p[3]);
  return buf;
}

bool mg_aton(struct mg_str str, struct mg_addr *addr) {
  uint8_t data[4] = {0, 0, 0, 0};
  // LOG(LL_INFO, ("[%.*s]", (int) str.len, str.ptr));
  if (!mg_casecmp(str.ptr, "localhost")) {
    addr->ip = mg_htonl(0x7f000001);
    return true;
  } else if (addr->is_ip6) {
    return false;
  } else {
    size_t i, num_octets = 0;
    // LOG(LL_DEBUG, ("[%.*s]", (int) str.len, str.ptr));
    for (i = 0; i < str.len; i++) {
      // LOG(LL_DEBUG,
      //("  %c %zu %hhu %zu", str.ptr[i], i, data[num_octets], num_octets));
      if (str.ptr[i] >= '0' && str.ptr[i] <= '9') {
        int octet = data[num_octets] * 10 + (str.ptr[i] - '0');
        if (octet > 255) return false;
        data[num_octets] = octet;
      } else if (str.ptr[i] == '.') {
        if (num_octets >= 3 || i == 0 || str.ptr[i - 1] == '.') return false;
        num_octets++;
      } else {
        return false;
      }
    }
    if (num_octets != 3 || str.ptr[i - 1] == '.') return false;
    memcpy(&addr->ip, data, sizeof(data));
    return true;
  }
}
