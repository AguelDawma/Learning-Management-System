#pragma once

#include "AppContext.hpp"

#include <optional>
#include <string>

// First-run set-up. Returns the temporary administrator password when a new
// administrator account had to be created, so the installer can show it once.
std::optional<std::string> ensureAdministrator(AppContext& ctx);

// Fills an empty database with one department, a course and demo accounts for
// each role (all with password "Password1", no forced change). For demos and the viva.
void seedDemoData(AppContext& ctx);
