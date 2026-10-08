// SPDX-License-Identifier: MPL-2.0

#pragma once

#define OPENEMULSION_VERSION_MAJOR 0
#define OPENEMULSION_VERSION_MINOR 44
#define OPENEMULSION_VERSION_PATCH 0
#define OPENEMULSION_STRINGIFY_IMPL(value) #value
#define OPENEMULSION_STRINGIFY(value) OPENEMULSION_STRINGIFY_IMPL(value)

namespace pluginversion {
inline constexpr unsigned Major = OPENEMULSION_VERSION_MAJOR;
inline constexpr unsigned Minor = OPENEMULSION_VERSION_MINOR;
inline constexpr unsigned Patch = OPENEMULSION_VERSION_PATCH;
inline constexpr const char* Label = OPENEMULSION_STRINGIFY(OPENEMULSION_VERSION_MAJOR) "."
    OPENEMULSION_STRINGIFY(OPENEMULSION_VERSION_MINOR) "." OPENEMULSION_STRINGIFY(OPENEMULSION_VERSION_PATCH);

// The OFX export has only two version fields; reserve three digits for patch releases.
constexpr unsigned ofxMinor(unsigned minor, unsigned patch) { return minor * 1000 + patch; }
inline constexpr unsigned OfxMinor = ofxMinor(Minor, Patch);
static_assert(Patch < 1000 && Minor <= 4294966, "Version exceeds the OFX encoding range");
}
