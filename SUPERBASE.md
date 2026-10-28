# Muzi Works SuperBase — MeshCore Companion Bluetooth

Candidata **v1.18-sb2-rc1** para **Muzi Works SuperBase con Base Uno
(nRF52840 + SX1262) y Super IO**. Conexión al móvil mediante la app MeshCore
por Bluetooth. Se publica como prerelease para validar un alcance congelado.

El perfil `muzi_base_uno_superIO_companion_radio_ble` es el nombre interno de
compilación heredado del soporte de hardware oficial; el producto es **Muzi Works
SuperBase**. No usar perfiles Base Duo/LR1121 en el modelo Base Uno/SX1262.

## Alcance

- Batería Bluetooth BAS con reintentos y nueva entrega al suscribirse.
- Corrección del aviso falso del cargador BQ25185.
- Bluetooth controlable desde la pantalla y preferencias persistentes.
- Modos de buzzer All, Off, Messages, System y DMs Only; reproducción estable.
- Navegación con seis controles, filtrado de rebotes y alternativa por sondeo.
- GPS sujeto a Mode 2 y a la preferencia GPS del menú.
- Reposo de sensores que no usa este Companion y preparación de apagado.
- Guardado de preferencias mediante archivo temporal y recuperación RX de radio.
- Corrección de la entrada DFU, reinicio posterior y compatibilidad con Nordic.
- Emojis monocromos en nombres, contactos y mensajes; catálogo de 3.953 secuencias.

No se añaden funciones en esta candidata. Se conserva el protocolo MeshCore y
la configuración guardada del usuario. No incluye teclado autónomo ni brújula.

## Estado de validación

La [matriz de implementación y pruebas](docs/superbase-rc1-validation.md) separa
pruebas automáticas, observaciones físicas anteriores y comprobaciones pendientes.
**La RC1 exacta no se ha instalado ni ensayado físicamente.** El paquete usa la
compilación de producción optimizada, sin interfaz Companion USB de diagnóstico.
Las imágenes debug y los registros internos no forman parte de esta publicación.

Los seis controles responden en la implementación ensayada, pero la captura por
interrupciones no está aprobada; el sondeo puede perder pulsaciones muy breves
entre consultas. Faltan pruebas prolongadas de autonomía, alcance y persistencia
en el binario final. Por eso esta candidata todavía no es una versión estable.

## Archivos e instalación

- `MeshCore-Muzi-Works-SuperBase-Companion-BLE-v1.18-sb2-rc1.uf2`: aplicación UF2.
- `MeshCore-Muzi-Works-SuperBase-Companion-BLE-v1.18-sb2-rc1-dfu.zip`: paquete
  Nordic DFU; seleccionar directamente este ZIP en la app de actualización.
- `SHA256SUMS.txt`, `BUILD.json` y `VALIDATION.json`: integridad, revisión de código
  y auditoría del paquete. Los archivos solo contienen la aplicación, sin cambiar
  bootloader ni SoftDevice.

Para UF2, conectar USB y entrar al cargador con doble RESET; copiar el archivo
UF2 a la unidad del nodo. Para DFU, usar la app Nordic existente con sus valores
predeterminados. No se requieren nuevas aplicaciones ni cambios de ajustes.
Después, conectar MeshCore con el PIN que aparece en la pantalla. El valor
alternativo de compilación `123456` no tiene por qué ser el PIN activo. Nordic
puede eliminar el vínculo al actualizar con valores predeterminados; si MeshCore
solicita emparejamiento, usar el PIN de la pantalla.

GPS requiere Mode 2 y habilitación en el menú. La recepción dentro de edificios
no está garantizada; se obtuvo fix a cielo abierto en la implementación ensayada.
La radio conserva los parámetros guardados. El valor de compilación para nodos
sin configurar sigue siendo 869.618 MHz; este fork no configura una región.

## Compilar y verificar

```text
pio run -e muzi_base_uno_superIO_companion_radio_ble -t buildprog -t create_uf2
python test/superbase/run.py
```

Linux/WSL también admite `python test/superbase/run.py --sanitize`. El flujo
**Build SuperBase Companion BLE** compila únicamente el perfil de producción,
ejecuta las regresiones y audita los límites de flash/RAM y la coincidencia de
aplicación UF2/DFU antes de producir el artefacto candidato.

El flujo queda incluido en la rama candidata, pero GitHub Actions no lo ejecutó
porque todavía no existe en la rama predeterminada del repositorio. Los archivos
de esta publicación se compilaron y auditaron localmente desde la revisión
publicada; las regresiones se ejecutaron en Windows y Linux/WSL. No se atribuye
un resultado de CI a esta publicación.

El soporte de hardware procede de MeshCore oficial, base
`81dbae592ffad40b4212bc0b37e82b38da08f4bc`. Las mejoras adaptadas se basan en
el alcance acordado del fork de Meshtastic, sin trasladar sus mecanismos de MQTT
ni sus cambios de seguridad específicos. La [atribución OpenMoji](src/helpers/ui/emoji/README.md)
y su licencia CC BY-SA 4.0 acompañan los pictogramas.
