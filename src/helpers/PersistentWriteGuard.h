#pragma once

// Board-specific power policy may override this weak hook. Writers must fail
// closed when persistent storage is unsafe rather than starting a flash write.
bool meshcorePersistentWritesAllowed();
