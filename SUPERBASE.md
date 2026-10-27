# Muzi Works SuperBase — MeshCore Companion Bluetooth

Este fork está preparado para el **SuperBase con Base Uno (nRF52840 + SX1262) y Super IO**, conectado a la aplicación MeshCore del móvil por Bluetooth.

El soporte de hardware procede de MeshCore oficial, incorporado en el [PR #3108](https://github.com/meshcore-dev/MeshCore/pull/3108). La base de esta rama es `81dbae592ffad40b4212bc0b37e82b38da08f4bc` de `meshcore-dev/MeshCore/main`.

## Perfil seleccionado

`muzi_base_uno_superIO_companion_radio_ble` es el entorno predeterminado de PlatformIO en esta rama. Incluye SX1262, Bluetooth, pantalla SH1107 de 128 × 128, navegación de la interfaz Companion con botones, GPS y buzzer. El interruptor en Mode 2 habilita el GPS. El código de emparejamiento configurado es `123456`; cámbialo desde la aplicación después de conectar.

Este perfil utiliza la interfaz Companion de MeshCore. No implementa el teclado autónomo ni la brújula de Meshtastic. Los mensajes se escriben desde el móvil. MeshCore y Meshtastic utilizan redes y protocolos distintos.

No selecciones los perfiles Base Duo (LR1121) para el SuperBase con Base Uno.

## Compilar en Windows

Con Python, Git y PlatformIO instalados, abre una terminal en la carpeta del repositorio:

```powershell
pio run -e muzi_base_uno_superIO_companion_radio_ble
pio run -e muzi_base_uno_superIO_companion_radio_ble -t create_uf2
```

El archivo instalable queda en:

```text
.pio/build/muzi_base_uno_superIO_companion_radio_ble/firmware.uf2
```

También puedes usar `pio run` para compilar únicamente el perfil predeterminado.

## Compilar en GitHub

El flujo **Build SuperBase Companion BLE** compila este perfil al subir cambios a esta rama y puede ejecutarse manualmente desde Actions. El artefacto descargable contiene el UF2 y el ZIP DFU generados por el proceso oficial `build.sh`, junto con sus sumas SHA-256. El ZIP descargado desde Actions es un contenedor: extráelo para obtener el UF2.

## Instalar y conectar

1. Guarda los ajustes de Meshtastic que quieras conservar antes de sustituir su firmware.
2. Conecta el SuperBase por USB con un cable de datos y pulsa RESET dos veces rápidamente para entrar al cargador UF2.
3. Cuando aparezca la unidad USB del cargador, copia `firmware.uf2` a esa unidad. El dispositivo debe reiniciarse al terminar. No copies el ZIP contenedor de Actions.
4. Abre la aplicación MeshCore, busca el dispositivo por Bluetooth y empareja con `123456`.
5. Configura la frecuencia, ancho de banda, spreading factor y coding rate que utilice tu red local, dentro de lo permitido en tu región. El valor predeterminado heredado del código es **869.618 MHz**; este fork no presupone una región de uso.
6. Comprueba pantalla, lectura de batería, conexión Bluetooth, GPS en Mode 2 y envío/recepción con otro nodo MeshCore que tenga los mismos parámetros de radio.

Si no aparece la unidad del cargador, consulta la [documentación del fabricante](https://muzi.works/pages/resources) antes de intentar cambiar el bootloader.

## Alcance de la validación

La compilación comprueba que el firmware se puede generar para el perfil seleccionado. El funcionamiento físico de radio, Bluetooth, pantalla, batería, GPS y buzzer requiere probarlo en un SuperBase real.

## Mantener el fork

El remoto `origin` apunta al fork personal y `upstream` a MeshCore oficial. Integra futuras actualizaciones oficiales mediante una rama y revisión de cambios; la rama principal y los demás trabajos del fork son independientes de esta preparación.

Fuentes: [SuperBase](https://muzi.works/products/superbase), [perfil oficial de hardware](https://github.com/meshcore-dev/MeshCore/blob/main/variants/muzi_base/platformio.ini), [MeshCore](https://github.com/meshcore-dev/MeshCore).
