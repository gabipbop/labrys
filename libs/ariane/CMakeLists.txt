# ARIANE : bibliothèque partagée de la suite LABRYS.
# Cible INTERFACE tant qu'aucun module n'est extrait ; elle deviendra STATIC
# avec le premier fichier source (voir docs/DECISIONS.md, ADR-002).

add_library(ariane INTERFACE)
add_library(labrys::ariane ALIAS ariane)
