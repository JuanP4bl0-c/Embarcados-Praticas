# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/juan/esp-idf/components/bootloader/subproject"
  "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader"
  "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader-prefix"
  "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader-prefix/tmp"
  "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader-prefix/src/bootloader-stamp"
  "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader-prefix/src"
  "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/42849DD9849DCFB1/ufc/2025.2/Embarcados/esp32/Projeto estufa Inteligente com MQTT WIP/Projeto/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
