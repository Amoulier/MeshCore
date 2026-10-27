# Muzi Works SuperBase — MeshCore Companion Bluetooth

Este fork está preparado para el **SuperBase con Base Uno (nRF52840 + SX1262) y Super IO**, conectado a la aplicación MeshCore del móvil por Bluetooth.

El soporte de hardware procede de MeshCore oficial, incorporado en el [PR #3108](https://github.com/meshcore-dev/MeshCore/pull/3108). La base de esta rama es `81dbae592ffad40b4212bc0b37e82b38da08f4bc` de `meshcore-dev/MeshCore/main`.

## Perfil seleccionado

`muzi_base_uno_superIO_companion_radio_ble` es el entorno predeterminado de PlatformIO en esta rama. Incluye SX1262, Bluetooth, pantalla SH1107 de 128 × 128, navegación de la interfaz Companion con botones, GPS y buzzer. Para usar GPS deben estar activados tanto Mode 2 como la opción GPS de la interfaz.

Usa el **PIN que aparece en la pantalla** para emparejar. MeshCore genera un PIN de sesión cuando la pantalla funciona y no existe un PIN guardado. `123456` es el valor alternativo de compilación para el funcionamiento sin pantalla; no es necesariamente el PIN activo del SuperBase.

Este perfil utiliza la interfaz Companion de MeshCore. No implementa el teclado autónomo ni la brújula de Meshtastic. Los mensajes se escriben desde el móvil. MeshCore y Meshtastic utilizan redes y protocolos distintos.

No selecciones los perfiles Base Duo (LR1121) para el SuperBase con Base Uno.

## Mejoras adaptadas del fork de Meshtastic

Referencia funcional: [Meshtastic SuperBase, revisión 223e443ae](https://github.com/Amoulier/meshtastic-superbase-firmware/tree/223e443ae). La implementación conserva el protocolo y la configuración de radio de MeshCore.

| Función | Comportamiento en este fork |
| --- | --- |
| Batería Bluetooth | Servicio estándar BAS, porcentaje limitado a 0–100, reintentos de escritura/notificación y actualización al reconectar o suscribirse tarde. Conserva la medición de voltaje original. |
| Cargador | Distingue carga, reposo, fallo recuperable y fallo persistente del BQ25185. Muestra aviso temporal y advertencia persistente, que desaparecen al recuperarse. Ignora los pines de fallo cuando no hay alimentación USB. |
| Bluetooth en ejecución | Encendido y apagado desde su página, sin reinicio, con preferencia conservada al arrancar de nuevo. |
| Buzzer | Página Buzzer: All, Off, Messages, System y DMs Only. El modo se guarda. DMs Only avisa de mensajes directos, incluso con el móvil conectado; los mensajes de canal se conservan sin interrumpir la pantalla. |
| Audio RTTTL | Copia estable de la melodía, control de propiedad y parada inmediata al silenciar; una instancia no puede detener el sonido de otra. |
| Navegación | Captura de pulsación y liberación por interrupciones, alternativa por sondeo, filtrado de rebotes y tiempos válidos al desbordarse el reloj. Se conservan las pulsaciones durante la espera del bucle. |
| GPS | Mode 2 y la preferencia de software deben permitirlo. Evita arranques duplicados e invalida el fix anterior del receptor al apagar o comenzar una búsqueda nueva. La posición guardada del nodo se conserva. |
| Energía | Solicita reposo al ICM20948 y al magnetómetro AK09916, sin uso en este Companion. Al apagar también deshabilita buzzer, alimentación de pantalla y GPS. |
| Preferencias y apagado | Escribe preferencias en un archivo temporal y las sustituye al terminar; ante fallo conserva el archivo anterior y reintenta. Vacía contactos pendientes y caché de flash antes de apagar o reiniciar. |
| Actualización Bluetooth | La entrada DFU espera a que el bucle principal termine y guarde los datos; rechaza la entrada si el guardado falla o no responde a tiempo. |
| Radio SX1262 | Reintentos espaciados, reinicialización tras fallos reiterados de recepción y restauración de frecuencia, ancho de banda, SF, CR, potencia y RX Boost. Protege las transmisiones activas. |

Los botones izquierda/derecha y arriba/abajo recorren las páginas. El botón central selecciona; Atrás regresa al inicio. Tres pulsaciones rápidas de Atrás cambian el modo del buzzer. La página Buzzer permite cambiarlo directamente con el botón central.

La regla de posición estacionaria de 12 horas de Meshtastic no necesita un temporizador equivalente: este Companion no genera envíos periódicos de posición por iniciativa propia. Se conservan los anuncios solicitados por el usuario y las respuestas de telemetría de MeshCore. No se trasladan el ACK por MQTT ni los cambios de seguridad específicos de Meshtastic.

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

Esta rama también corrige la generación UF2 en carpetas con espacios y propaga los errores de conversión a PlatformIO.

## Compilar en GitHub

El flujo **Build SuperBase Companion BLE** está configurado para compilar este perfil al subir cambios a esta rama. Incluye un disparador manual para cuando el flujo esté disponible en la rama predeterminada del fork. El artefacto descargable contiene el UF2 y el ZIP DFU generados por el proceso oficial `build.sh`, junto con sus sumas SHA-256. El ZIP descargado desde Actions es un contenedor: extráelo para obtener el UF2.

## Instalar y conectar

1. Guarda los ajustes de Meshtastic que quieras conservar antes de sustituir su firmware.
2. Conecta el SuperBase por USB con un cable de datos y pulsa RESET dos veces rápidamente para entrar al cargador UF2.
3. Cuando aparezca la unidad USB del cargador, copia `firmware.uf2` a esa unidad. El dispositivo debe reiniciarse al terminar. No copies el ZIP contenedor de Actions.
4. Abre la aplicación MeshCore, busca el dispositivo por Bluetooth y empareja con el PIN mostrado en la pantalla.
5. Configura la frecuencia, ancho de banda, spreading factor y coding rate que utilice tu red local, dentro de lo permitido en tu región. El valor predeterminado heredado del código es **869.618 MHz**; este fork no presupone una región de uso.
6. Comprueba pantalla, lectura de batería, conexión Bluetooth, GPS en Mode 2 y envío/recepción con otro nodo MeshCore que tenga los mismos parámetros de radio.

Si no aparece la unidad del cargador, consulta la [documentación del fabricante](https://muzi.works/pages/resources) antes de intentar cambiar el bootloader.

## Alcance de la validación

**v1.18-sb2** corrige una omisión de v1.18-sb1: la entrada STAT1 del cargador necesitaba `INPUT_PULLUP`. Al dejarla flotante podía aparecer `! Check charger` sin un fallo real. Se restauró la configuración eléctrica utilizada por nuestro fork de Meshtastic y se añadió una regresión de entrada de drenador abierto: reproduce el aviso falso sin pull-up y comprueba carga, reposo y fallos reales con pull-up. Compilación, regresiones Windows y Linux/WSL con AddressSanitizer/UBSan aprobadas. RAM: 147872 bytes; flash: 361280 bytes. Falta confirmar en el nodo físico si ésta era la causa de su aviso.

Validación local del 27 de octubre de 2026, versión **v1.18-sb1**: compilación del perfil y regresiones de producción en Windows y Linux/WSL para fallos del servicio BAS, estados del cargador, políticas y propiedad del audio, fallos de guardado, pulsaciones breves/rebotes/desbordamiento del reloj, recuperación RX, protección TX y espera/rechazo de DFU. Ejecución: `python test/superbase/run.py` con g++ instalado; Linux también pasó con `--sanitize` (AddressSanitizer y UBSan). La generación UF2 desde rutas con espacios y la propagación de errores de conversión también se comprobaron.

Uso de RAM: 147872 / 248832 bytes (59.4 %). Uso de flash: 361264 / 708608 bytes (51.0 %). La auditoría `bin/audit-superbase-package.py` comprobó familia nRF52840, límites de flash de aplicación, vectores de arranque, versión, CRC16, requisito SoftDevice y coincidencia de los datos de aplicación entre UF2 y ZIP DFU.

Las regresiones usan periféricos y almacenamiento simulados; no simulan el dispositivo completo. Falta comprobar en un SuperBase real los botones, pantalla, GPS, entrega de notificaciones BAS a un móvil, condiciones de fallo del cargador, actualización DFU, RF y consumo. No se ha instalado esta versión en el dispositivo durante este trabajo. La protección del archivo de preferencias no implica una transacción atómica conjunta de todos los archivos.

## Mantener el fork

El remoto `origin` apunta al fork personal y `upstream` a MeshCore oficial. Integra futuras actualizaciones oficiales mediante una rama y revisión de cambios; la rama principal y los demás trabajos del fork son independientes de esta preparación.

Fuentes: [SuperBase](https://muzi.works/products/superbase), [perfil oficial de hardware](https://github.com/meshcore-dev/MeshCore/blob/main/variants/muzi_base/platformio.ini), [MeshCore](https://github.com/meshcore-dev/MeshCore).
