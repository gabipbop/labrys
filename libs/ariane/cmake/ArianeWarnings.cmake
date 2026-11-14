# Warnings stricts pour le code LABRYS (pas pour les dépendances).
# Usage : target_link_libraries(<cible> PRIVATE ariane_warnings)

add_library(ariane_warnings INTERFACE)
target_compile_options(ariane_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
if(LABRYS_WARNINGS_AS_ERRORS)
    target_compile_options(ariane_warnings INTERFACE -Werror)
endif()
