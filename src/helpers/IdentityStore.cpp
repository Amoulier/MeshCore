#include "IdentityStore.h"

#include <stdio.h>
#include <string.h>
#include <helpers/PersistentWriteGuard.h>

namespace {

void makeIdentityPath(char *path, size_t path_size, const char *dir, const char *name, const char *suffix)
{
  snprintf(path, path_size, "%s/%s.id%s", dir, name, suffix);
}

bool loadIdentityFile(FILESYSTEM *fs, const char *path, mesh::LocalIdentity &id,
                      char *display_name = NULL, int max_name_size = 0)
{
  if (!fs->exists(path)) {
    return false;
  }

#if defined(RP2040_PLATFORM)
  File file = fs->open(path, "r");
#else
  File file = fs->open(path);
#endif
  if (!file) {
    return false;
  }

  const bool loaded = id.readFrom(file);
  if (loaded && display_name != NULL && max_name_size > 0) {
    const int bytes_to_read = max_name_size > 32 ? 32 : max_name_size;
    memset(display_name, 0, max_name_size);
    const int read_count = file.read(reinterpret_cast<uint8_t *>(display_name), bytes_to_read);
    if (read_count > 0) {
      display_name[bytes_to_read - 1] = 0;
    }
  }
  file.close();
  return loaded;
}

bool writeIdentityFile(FILESYSTEM *fs, const char *path, const mesh::LocalIdentity &id,
                       const char *display_name)
{
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  fs->remove(path);
  File file = fs->open(path, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = fs->open(path, "w");
#else
  File file = fs->open(path, "w", true);
#endif
  if (!file) {
    return false;
  }

  bool success = id.writeTo(file);
  if (success && display_name != NULL) {
    uint8_t encoded_name[32] = {};
    size_t name_length = strlen(display_name);
    if (name_length >= sizeof(encoded_name)) {
      name_length = sizeof(encoded_name) - 1;
    }
    memcpy(encoded_name, display_name, name_length);
    success = file.write(encoded_name, sizeof(encoded_name)) == sizeof(encoded_name);
  }
  file.flush();
  file.close();
  return success;
}

bool commitIdentityAtomically(FILESYSTEM *fs, const char *path, const mesh::LocalIdentity &id,
                              const char *display_name)
{
  if (!meshcorePersistentWritesAllowed()) {
    return false;
  }

  char temporary_path[64];
  char backup_path[64];
  snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path);
  snprintf(backup_path, sizeof(backup_path), "%s.bak", path);
  fs->remove(temporary_path);

  if (!writeIdentityFile(fs, temporary_path, id, display_name)) {
    fs->remove(temporary_path);
    return false;
  }

  mesh::LocalIdentity verified;
  bool key_pair_verified = loadIdentityFile(fs, temporary_path, verified) && verified.matches(id);
  if (key_pair_verified) {
    static const uint8_t verification_message[] = {
      'M', 'e', 's', 'h', 'C', 'o', 'r', 'e', '-', 'i', 'd', 'e', 'n', 't', 'i', 't', 'y'
    };
    uint8_t verification_signature[SIGNATURE_SIZE] = {};
    verified.sign(verification_signature, verification_message, sizeof(verification_message));
    key_pair_verified = verified.verify(verification_signature, verification_message,
                                        sizeof(verification_message));
  }
  if (!key_pair_verified || !meshcorePersistentWritesAllowed()) {
    fs->remove(temporary_path);
    return false;
  }

  fs->remove(backup_path);
  const bool had_primary = fs->exists(path);
  if (had_primary && !fs->rename(path, backup_path)) {
    fs->remove(temporary_path);
    return false;
  }

  if (!fs->rename(temporary_path, path)) {
    if (had_primary) {
      fs->rename(backup_path, path);
    }
    fs->remove(temporary_path);
    return false;
  }

  return true;
}

} // namespace

bool IdentityStore::load(const char *name, mesh::LocalIdentity &id)
{
  char path[64];
  char backup_path[64];
  char temporary_path[64];
  makeIdentityPath(path, sizeof(path), _dir, name, "");
  makeIdentityPath(backup_path, sizeof(backup_path), _dir, name, ".bak");
  makeIdentityPath(temporary_path, sizeof(temporary_path), _dir, name, ".tmp");
  return loadIdentityFile(_fs, path, id) || loadIdentityFile(_fs, backup_path, id) ||
         loadIdentityFile(_fs, temporary_path, id);
}

bool IdentityStore::load(const char *name, mesh::LocalIdentity &id, char display_name[], int max_name_size)
{
  char path[64];
  char backup_path[64];
  char temporary_path[64];
  makeIdentityPath(path, sizeof(path), _dir, name, "");
  makeIdentityPath(backup_path, sizeof(backup_path), _dir, name, ".bak");
  makeIdentityPath(temporary_path, sizeof(temporary_path), _dir, name, ".tmp");
  return loadIdentityFile(_fs, path, id, display_name, max_name_size) ||
         loadIdentityFile(_fs, backup_path, id, display_name, max_name_size) ||
         loadIdentityFile(_fs, temporary_path, id, display_name, max_name_size);
}

bool IdentityStore::save(const char *name, const mesh::LocalIdentity &id)
{
  char path[64];
  makeIdentityPath(path, sizeof(path), _dir, name, "");
  const bool success = commitIdentityAtomically(_fs, path, id, NULL);
  MESH_DEBUG_PRINTLN("IdentityStore::save() atomic write - %s", success ? "OK" : "Err");
  return success;
}

bool IdentityStore::save(const char *name, const mesh::LocalIdentity &id, const char display_name[])
{
  char path[64];
  makeIdentityPath(path, sizeof(path), _dir, name, "");
  return commitIdentityAtomically(_fs, path, id, display_name);
}
