#include "assets.h"

#include <SDL.h>
#include <vector>

#ifndef PKGDATADIR
#define PKGDATADIR ""
#endif

std::string AssetPath(const std::string& name) {
    std::vector<std::string> candidates;
    if (char* base = SDL_GetBasePath()) {
        candidates.push_back(std::string(base) + "assets/" + name);
        SDL_free(base);
    }
    candidates.push_back("assets/" + name);
    if (PKGDATADIR[0] != '\0') {
        candidates.push_back(std::string(PKGDATADIR) + "/assets/" + name);
    }
    for (const auto& path : candidates) {
        if (SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb")) {
            SDL_RWclose(rw);
            return path;
        }
    }
    return "assets/" + name;
}
