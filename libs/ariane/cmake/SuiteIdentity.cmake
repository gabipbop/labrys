# Identité de la suite LABRYS : seul endroit où les noms et codes sont définis.
# Pour renommer un instrument, on ne modifie que ce fichier.

set(LABRYS_SUITE_NAME "LABRYS")
set(LABRYS_COMPANY_NAME "Raw Powa Studio")
set(LABRYS_MANUFACTURER_CODE "Rwps")
set(LABRYS_BUNDLE_ID_PREFIX "com.rawpowastudio")

# Instruments : <ID>_DISPLAY_NAME (UI), <ID>_FILE_NAME (fichiers, cibles), <ID>_PLUGIN_CODE (4 caractères).
set(LABRYS_INSTRUMENTS DEDALE TALOS ICARE EUROPE PHEDRE)

set(DEDALE_DISPLAY_NAME "DÉDALE")
set(DEDALE_FILE_NAME "Dedale")
set(DEDALE_PLUGIN_CODE "Ddle")

set(TALOS_DISPLAY_NAME "TALOS")
set(TALOS_FILE_NAME "Talos")
set(TALOS_PLUGIN_CODE "Tals")

set(ICARE_DISPLAY_NAME "ICARE")
set(ICARE_FILE_NAME "Icare")
set(ICARE_PLUGIN_CODE "Icar")

set(EUROPE_DISPLAY_NAME "EUROPE")
set(EUROPE_FILE_NAME "Europe")
set(EUROPE_PLUGIN_CODE "Eurp")

set(PHEDRE_DISPLAY_NAME "PHÈDRE")
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
    set(${_id}_BUNDLE_ID "${LABRYS_BUNDLE_ID_PREFIX}.${${_id}_FILE_NAME}")
    string(TOLOWER "${${_id}_BUNDLE_ID}" ${_id}_BUNDLE_ID)
endforeach()
unset(_labrys_seen_codes)
unset(_code)
