#include "ClientACL.h"

#include <helpers/PersistentWriteGuard.h>

namespace {

static File openWrite(FILESYSTEM *fs, const char *filename)
{
  if (!meshcorePersistentWritesAllowed()) {
    return File();
  }
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  fs->remove(filename);
  return fs->open(filename, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  return fs->open(filename, "w");
#else
  return fs->open(filename, "w", true);
#endif
}

static const char *readableAclPath(FILESYSTEM *fs)
{
  if (fs->exists("/s_contacts")) {
    return "/s_contacts";
  }
  return fs->exists("/s_contacts.bak") ? "/s_contacts.bak" : "/s_contacts";
}

static bool commitAcl(FILESYSTEM *fs)
{
  if (!meshcorePersistentWritesAllowed()) {
    fs->remove("/s_contacts.tmp");
    return false;
  }

  fs->remove("/s_contacts.bak");
  const bool had_primary = fs->exists("/s_contacts");
  if (had_primary && !fs->rename("/s_contacts", "/s_contacts.bak")) {
    fs->remove("/s_contacts.tmp");
    return false;
  }

  if (!fs->rename("/s_contacts.tmp", "/s_contacts")) {
    if (had_primary) {
      fs->rename("/s_contacts.bak", "/s_contacts");
    }
    fs->remove("/s_contacts.tmp");
    return false;
  }
  return true;
}

} // namespace

void ClientACL::load(FILESYSTEM *fs, const mesh::LocalIdentity &self_id)
{
  _fs = fs;
  num_clients = 0;
  const char *path = readableAclPath(_fs);
  if (!_fs->exists(path)) {
    return;
  }

#if defined(RP2040_PLATFORM)
  File file = _fs->open(path, "r");
#else
  File file = _fs->open(path);
#endif
  if (!file) {
    return;
  }

  bool full = false;
  while (!full) {
    ClientInfo c;
    uint8_t pub_key[32];
    uint8_t unused[2];

    memset(&c, 0, sizeof(c));

    bool success = (file.read(pub_key, 32) == 32);
    success = success && (file.read((uint8_t *)&c.permissions, 1) == 1);
    success = success && (file.read((uint8_t *)&c.extra.room.sync_since, 4) == 4);
    success = success && (file.read(unused, 2) == 2);
    success = success && (file.read((uint8_t *)&c.out_path_len, 1) == 1);
    success = success && (file.read(c.out_path, 64) == 64);
    success = success && (file.read(c.shared_secret, PUB_KEY_SIZE) == PUB_KEY_SIZE);

    if (!success) {
      break;
    }

    c.id = mesh::Identity(pub_key);
    self_id.calcSharedSecret(c.shared_secret, pub_key);
    if (num_clients < MAX_CLIENTS) {
      clients[num_clients++] = c;
    } else {
      full = true;
    }
  }
  file.close();
}

void ClientACL::save(FILESYSTEM *fs, bool (*filter)(ClientInfo *))
{
  _fs = fs;
  if (!meshcorePersistentWritesAllowed()) {
    return;
  }

  _fs->remove("/s_contacts.tmp");
  File file = openWrite(_fs, "/s_contacts.tmp");
  if (!file) {
    return;
  }

  uint8_t unused[2] = {};
  bool success = true;
  for (int i = 0; i < num_clients; i++) {
    auto c = &clients[i];
    if (c->permissions == 0 || (filter && !filter(c))) {
      continue;
    }

    success = (file.write(c->id.pub_key, 32) == 32);
    success = success && (file.write((uint8_t *)&c->permissions, 1) == 1);
    success = success && (file.write((uint8_t *)&c->extra.room.sync_since, 4) == 4);
    success = success && (file.write(unused, 2) == 2);
    success = success && (file.write((uint8_t *)&c->out_path_len, 1) == 1);
    success = success && (file.write(c->out_path, 64) == 64);
    success = success && (file.write(c->shared_secret, PUB_KEY_SIZE) == PUB_KEY_SIZE);

    if (!success) {
      break;
    }
  }

  file.flush();
  file.close();
  if (!success || !commitAcl(_fs)) {
    _fs->remove("/s_contacts.tmp");
  }
}

bool ClientACL::clear()
{
  if (!_fs || !meshcorePersistentWritesAllowed()) {
    return false;
  }
  _fs->remove("/s_contacts.tmp");
  _fs->remove("/s_contacts.bak");
  if (_fs->exists("/s_contacts")) {
    _fs->remove("/s_contacts");
  }
  memset(clients, 0, sizeof(clients));
  num_clients = 0;
  return true;
}

ClientInfo *ClientACL::getClient(const uint8_t *pubkey, int key_len)
{
  for (int i = 0; i < num_clients; i++) {
    if (memcmp(pubkey, clients[i].id.pub_key, key_len) == 0) {
      return &clients[i];
    }
  }
  return NULL;
}

ClientInfo *ClientACL::putClient(const mesh::Identity &id, uint8_t init_perms)
{
  uint32_t min_time = 0xFFFFFFFF;
  ClientInfo *oldest = &clients[MAX_CLIENTS - 1];
  for (int i = 0; i < num_clients; i++) {
    if (id.matches(clients[i].id)) {
      return &clients[i];
    }
    if (!clients[i].isAdmin() && clients[i].last_activity < min_time) {
      oldest = &clients[i];
      min_time = oldest->last_activity;
    }
  }

  ClientInfo *c;
  if (num_clients < MAX_CLIENTS) {
    c = &clients[num_clients++];
  } else {
    c = oldest;
  }
  memset(c, 0, sizeof(*c));
  c->permissions = init_perms;
  c->id = id;
  c->out_path_len = OUT_PATH_UNKNOWN;
  return c;
}

bool ClientACL::applyPermissions(const mesh::LocalIdentity &self_id, const uint8_t *pubkey, int key_len, uint8_t perms)
{
  ClientInfo *c;
  if ((perms & PERM_ACL_ROLE_MASK) == PERM_ACL_GUEST) {
    c = getClient(pubkey, key_len);
    if (c == NULL) {
      return false;
    }

    num_clients--;
    int i = c - clients;
    while (i < num_clients) {
      clients[i] = clients[i + 1];
      i++;
    }
  } else {
    if (key_len < PUB_KEY_SIZE) {
      return false;
    }

    mesh::Identity id(pubkey);
    c = putClient(id, 0);

    c->permissions = perms;
    self_id.calcSharedSecret(c->shared_secret, pubkey);
  }
  return true;
}
