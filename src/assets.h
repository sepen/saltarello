#ifndef SALTARELLO_ASSETS_H
#define SALTARELLO_ASSETS_H

#include <string>

// Resolve an asset path: next to the executable, then the current directory,
// then the install location ($(pkgdatadir)).
std::string AssetPath(const std::string& name);

#endif
