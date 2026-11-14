# Identité de la suite LABRYS : seul endroit où les noms et codes sont définis.
# Pour renommer un instrument, on ne modifie que ce fichier.

set(LABRYS_SUITE_NAME "LABRYS")
set(LABRYS_COMPANY_NAME "Raw Powa Studio")
set(LABRYS_MANUFACTURER_CODE "Rwps")
set(LABRYS_BUNDLE_ID_PREFIX "com.rawpowastudio")

# Instruments :
#   <ID>_DISPLAY_NAME  nom affiché dans l'interface du plugin (accents permis)
#   <ID>_HOST_NAME     nom vu par la DAW, ASCII obligatoire : un nom accentué empêche le VST3 de se charger
#                      dans les hôtes JUCE (pluginval) et s'affiche mal ailleurs (ADR-007)
#   <ID>_FILE_NAME     fichiers et cibles CMake
#   <ID>_PLUGIN_CODE   4 caractères ASCII
set(LABRYS_INSTRUMENTS DEDALE TALOS ICARE EUROPE PHEDRE)

set(DEDALE_DISPLAY_NAME "DÉDALE")
set(DEDALE_HOST_NAME "DEDALE")
set(DEDALE_FILE_NAME "Dedale")
set(DEDALE_PLUGIN_CODE "Ddle")

set(TALOS_DISPLAY_NAME "TALOS")
set(TALOS_HOST_NAME "TALOS")
set(TALOS_FILE_NAME "Talos")
set(TALOS_PLUGIN_CODE "Tals")

set(ICARE_DISPLAY_NAME "ICARE")
set(ICARE_HOST_NAME "ICARE")
set(ICARE_FILE_NAME "Icare")
set(ICARE_PLUGIN_CODE "Icar")

set(EUROPE_DISPLAY_NAME "EUROPE")
set(EUROPE_HOST_NAME "EUROPE")
set(EUROPE_FILE_NAME "Europe")
set(EUROPE_PLUGIN_CODE "Eurp")

set(PHEDRE_DISPLAY_NAME "PHÈDRE")
set(PHEDRE_HOST_NAME "PHEDRE")
set(PHEDRE_FILE_NAME "Phedre")
set(PHEDRE_PLUGIN_CODE "Phed")

# Garde-fous : les hôtes AU/VST3 exigent des codes de 4 caractères ASCII, uniques dans la suite.
set(_labrys_seen_codes "")
if(NOT LABRYS_MANUFACTURER_CODE MATCHES "^[A-Za-z0-9][A-Za-z0-9][A-Za-z0-9][A-Za-z0-9]$")
    message(FATAL_ERROR "Code fabricant invalide : '${LABRYS_MANUFACTURER_CODE}' (4 caractères ASCII attendus)")
endif()
foreach(_id IN LISTS LABRYS_INSTRUMENTS)
    set(_code "${${_id}_PLUGIN_CODE}")
    if(NOT _code MATCHES "^[A-Za-z0-9][A-Za-z0-9][A-Za-z0-9][A-Za-z0-9]$")
        message(FATAL_ERROR "Code plugin invalide pour ${_id} : '${_code}' (4 caractères ASCII attendus)")
    endif()
    if(_code IN_LIST _labrys_seen_codes)
        message(FATAL_ERROR "Code plugin en double : '${_code}' (${_id})")
    endif()
    list(APPEND _labrys_seen_codes "${_code}")
    if(NOT "${${_id}_HOST_NAME}" MATCHES "^[A-Za-z0-9 _-]+$")
        message(FATAL_ERROR "Nom hôte non ASCII pour ${_id} : '${${_id}_HOST_NAME}'")
    endif()
    set(${_id}_BUNDLE_ID "${LABRYS_BUNDLE_ID_PREFIX}.${${_id}_FILE_NAME}")
    string(TOLOWER "${${_id}_BUNDLE_ID}" ${_id}_BUNDLE_ID)
endforeach()
unset(_labrys_seen_codes)
unset(_code)
