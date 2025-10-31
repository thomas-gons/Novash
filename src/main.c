/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "shell/shell.h"
#include "utils/log.h"


int main() {
    if (shell_init() != 0) {
        return EXIT_FAILURE;
    }

    int exit_code = shell_loop();
    shell_cleanup();

    return exit_code;
}