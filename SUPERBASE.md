# Muzi Works SuperBase: MeshCore Companion Bluetooth

Candidata **v1.18-sb2-rc1** para **Muzi Works SuperBase con Base Uno
(nRF52840 + SX1262) y Super IO**, conectada al móvil mediante MeshCore Bluetooth.
**Candidata RC1 con validación funcional completada; no se presenta como una
versión estable general.** La sesión de 30 minutos a batería pasó, junto con
OTA, BAS nativo y los modos selectivos del buzzer descritos abajo.

El perfil `muzi_base_uno_superIO_companion_radio_ble` es el nombre interno de
compilación del hardware. El producto es Muzi Works SuperBase. No usar perfiles
Base Duo/LR1121 en este modelo Base Uno/SX1262.

## Funciones y controles

- Batería Bluetooth BAS con reintentos y entrega al suscribirse.
- Corrección del aviso falso del cargador BQ25185.
- Bluetooth desde pantalla y preferencias persistentes.
- Buzzer con All, Off, Messages, System y DMs Only.
- Seis controles con captura de flancos, filtrado y sondeo alternativo.
- Atrás para apagar la pantalla y centro para despertarla.
- LEDs On/Off persistentes e iconos de estado para LEDs/Buzzer.
- GPS exclusivamente por interruptor: Mode 1 apagado y Mode 2 encendido.
- Reposo de sensores sin uso, preparación de apagado y guardado de preferencias.
- Recuperación de radio y actualización Bluetooth con vínculo conservado.
- Emojis monocromos: 3.953 secuencias OpenMoji.

Con la pantalla apagada, centro la despierta y consume esa pulsación. Con ella
encendida, las direcciones recorren las páginas y centro ejecuta su acción.
Una pulsación corta de **Atrás apaga la pantalla** y la deja apagada si ya lo
estaba. Espera aproximadamente 280 ms para reconocer múltiples clics. Triple
Atrás y la pulsación larga conservan sus funciones existentes.

En **LEDs**, centro alterna On/Off y guarda la preferencia. On permite el
parpadeo verde y Off lo suprime; no apaga Bluetooth ni la radio. En **Buzzer**,
centro recorre los cinco modos. Los iconos de ambas páginas son de 32 × 32,
con variantes activadas/desactivadas y una sola leyenda, en la misma disposición
que Bluetooth. Su aspecto y funcionamiento se confirmaron en la pantalla física.

| Modo de buzzer | Avisos que permite |
| --- | --- |
| All | Mensajes y sistema |
| Off | Ninguno |
| Messages | Mensajes, incluidos DMs y textos de canal |
| System | Sistema; no mensajes |
| DMs Only | Mensajes directos |

El **GPS obedece exclusivamente al interruptor físico**, también al arrancar.
Su página muestra estado y satélites; centro no cambia la alimentación del GPS.
Las solicitudes de activación por software se rechazan. El dato heredado de
preferencia se conserva por compatibilidad y deja de actuar como control.
La recepción dentro de edificios no está garantizada.

### Límites del control de LEDs

La opción controla el verde P1.03/GPIO 35 y el azul P1.04/GPIO 36 durante
MeshCore; el azul continúa apagado conforme a la configuración del fork.
El rojo D4 de carga depende de `BAT_CHG_STATUS`, conectado a STAT2 del BQ25185,
y continúa bajo control del cargador de batería. Lo muestra el
[esquema oficial, página 1](https://cdn.shopify.com/s/files/1/0657/6973/4201/files/Base_Duo_Schematic_Rev01.pdf?v=1766439439).
Los destellos del cargador de arranque durante recuperación o actualización
también quedan fuera de LEDs Off.

## Validación de la candidata

La [matriz de implementación y pruebas](docs/superbase-rc1-validation.md) conserva
el ámbito de cada resultado. El instalable final ensayado e instalado es:

- ZIP SHA-256: `637f60b169b55d7fa44e107a20943841db0354b34793acf6b3ea0c133dd42f4b`.
- UF2 SHA-256: `7601f365ad1bf626cc725eb2939e9c9250aecf00dd2605846fb6c3abaa508434`.
- Código compilado: `e8d3ef4d3f31711a857f8925a2e9435b3a868047`.

La aplicación ocupa 452.152 bytes, compila con optimización de producción y
pasa la auditoría de coincidencia UF2/DFU y límites de flash/RAM. No incluye
comandos USB de diagnóstico ni trazas internas. Las regresiones del código
funcional pasaron en Windows y Linux/WSL con ASan/UBSan.

Comprobaciones físicas completadas en ese archivo:

- **OTA:** Nordic predeterminado transfirió 452.152 bytes, validó y activó.
  USB regresó automáticamente en 49,27 s. La primera reconexión normal de
  MeshCore quedó cifrada y vinculada, sin PIN ni eventos nuevos de
  emparejamiento, sin RESET manual ni reconectar USB. No se cambiaron ajustes
  de Nordic. Este ciclo no se compara contra una marca BONDED antigua.
- **BAS nativo Windows:** tres reconexiones, seis lecturas y seis notificaciones
  reales; cero notificaciones en las ventanas posteriores a desuscribirse.
  Se usaron adaptador y vínculo existentes, con ajustes intactos. El nivel
  observado de 84% no es una calibración ni una medida de autonomía.
- **Buzzer selectivo y persistencia tras RESET:** Messages arrancó en silencio
  y avisó con DM/canal; System sonó al arrancar y dejó DM/canal en silencio;
  DMs Only arrancó en silencio, avisó con DM y silenció canal. Se confirmó
  recepción en el nodo. El canal privado temporal de los dos nodos propios
  ya se retiró de ambos, conservando sus entradas anteriores.
- **Páginas LEDs/Buzzer:** ambos estados y su funcionamiento se confirmaron
  físicamente. Al terminar se confirmaron LEDs Off y Buzzer All.
- Instalación USB y retorno automático al firmware.

**Chrome Android no tiene aprobada la reconexión BAS:** las primeras lecturas,
notificaciones y paradas pasaron, pero en dos intentos la lectura después de
reconectar agotó el tiempo de espera. Es una limitación observada en ese
recorrido de cliente; no se ha demostrado su causa ni se atribuye al firmware.
El mismo instalable sí pasó la prueba nativa Windows descrita arriba.

GPS y radio conservan su evidencia de la implementación
`13317e834cd4835b58febddcc82d2fca8ac0700d77c635b52af3423615c8230f`:
GPS por switch, centro informativo, arranques a batería en ambos modos y fix
exterior con ocho satélites; radio bidireccional con Heltec V4, mensajes
confirmados y observados en ambos receptores. Los cambios posteriores de
iconos no modificaron ese código. Son resultados de la imagen identificada,
no mediciones nuevas del instalable final.

**PASS:** sesión continua de 30 minutos a batería en el instalable final, con 61 lecturas válidas, 44 notificaciones reales y cero desconexiones inesperadas. USB permaneció ausente, no se reconectó automáticamente y no hubo RESET. La conexión de prueba se cerró correctamente; los estados Bluetooth de Windows permanecieron intactos. El nivel observado osciló entre 82% y 83%. Es una comprobación de continuidad, no una medida de autonomía total.
No se han medido consumo instrumental, descarga completa, alcance máximo,
precisión/retención RTC ni fallos eléctricos inducidos. No se prometen esas
prestaciones ni compatibilidad con todos los clientes a partir de estas pruebas.
Se mantiene la identificación de candidata RC1 y sus límites de validación.

## Archivos e instalación

Archivos de la entrega:

- `MeshCore-Muzi-Works-SuperBase-Companion-BLE-v1.18-sb2-rc1.uf2`: aplicación USB.
- `MeshCore-Muzi-Works-SuperBase-Companion-BLE-v1.18-sb2-rc1-dfu.zip`: ZIP de aplicación para Nordic DFU.
- `Muzi-Works-SuperBase-Bootloader-v2.3-muzi.2-rc1.uf2`: cargador compatible.
- Fuente, matriz de validación, atribuciones, licencias y `SHA256SUMS.txt`.

La OTA con conservación del vínculo requiere **v2.3-muzi.2-rc1**, cuyo UF2 tiene
SHA-256 `5d8163cc5a4f28bef2639388c83a95c1b846ab4b8a2222efe963d8d0eda35114`.
El cargador anterior `v2.3-muzi.1` no tiene aprobado ese flujo. La firma de su
intercambio se reconoce de forma exacta; un nombre de versión no acredita por
sí solo otra compilación. S140 6.1.1 se conserva.

Los instalables de aplicación no contienen cargador ni SoftDevice. El UF2 del
cargador tampoco incluye la aplicación ni reemplaza SoftDevice. Son archivos
distintos y no son intercambiables.

Para instalar por USB, conectar el nodo, entrar al cargador con doble RESET y
comprobar `Board-ID: muzi-Base-Board` en `INFO_UF2.TXT`. Si corresponde actualizar
el cargador, copiar su UF2 y esperar a que termine; después volver a recuperación
cuando sea necesario e instalar la aplicación. Con el cargador compatible ya
instalado, basta actualizar la aplicación.

Para OTA, usar Nordic en sus valores predeterminados y seleccionar directamente
el ZIP de aplicación. No requiere nuevas apps ni cambios de ajustes del teléfono.
El emparejamiento inicial de MeshCore usa el PIN de la pantalla; el alternativo
de compilación `123456` puede diferir del activo. **Una solicitud de volver a
emparejar tras OTA se registra como fallo**, no como paso normal.

La radio conserva los parámetros guardados. El valor de compilación para nodos
sin configurar sigue siendo 869.618 MHz; este fork no selecciona una región.
No incorpora teclado autónomo, brújula, ACK por MQTT ni cambios de seguridad
específicos de Meshtastic.

## Compilar, fuentes y licencias

```text
pio run -e muzi_base_uno_superIO_companion_radio_ble -t buildprog -t create_uf2
python test/superbase/run.py
```

Linux/WSL admite `python test/superbase/run.py --sanitize`. El flujo preparado
**Build SuperBase Companion BLE** compila, ejecuta regresiones y audita paquetes.
Las compilaciones y pruebas documentadas de los instalables son locales; los
resultados de CI se identifican por separado en BUILD.json de la entrega.
No se reemplaza el archivo ensayado por una recompilación con otros bytes.

El soporte de hardware procede de MeshCore oficial, base
`81dbae592ffad40b4212bc0b37e82b38da08f4bc`. Los iconos de LEDs/Buzzer derivan de
Google Material Icons 3.0.1, Apache 2.0; el
[README del artwork](examples/companion_radio/ui-new/icons/material/README.md)
conserva SVG, atribución y modificaciones. La
[atribución OpenMoji](src/helpers/ui/emoji/README.md) y su licencia CC BY-SA 4.0
acompañan los emojis. Las imágenes debug, trazas y datos del nodo quedan fuera
de la distribución.

La entrega incluye fuente de MeshCore y el core LGPL del framework por separado.
Los avisos de terceros identifican versiones y reconstrucción, incluida una
pieza no LGPL de Nordic que debe obtenerse de su origen exacto y conforme a sus
condiciones para completar el framework. Ese archivo no se incluye bajo un
permiso de redistribución supuesto. Consulta SOURCE-AND-REBUILD.md dentro del
ZIP de avisos; las licencias de cada componente conservan su alcance.
