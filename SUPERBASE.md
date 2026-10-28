# Muzi Works SuperBase: MeshCore Companion Bluetooth

Candidata **v1.18-sb2-rc1** para **Muzi Works SuperBase con Base Uno
(nRF52840 + SX1262) y Super IO**, conectada al móvil mediante MeshCore Bluetooth.
**Todavía no está publicada ni aprobada como estable.** La publicación permanece
detenida mientras se completan las validaciones pendientes.

El perfil `muzi_base_uno_superIO_companion_radio_ble` es el nombre interno de
compilación heredado del soporte de hardware; el producto es **Muzi Works
SuperBase**. No usar perfiles Base Duo/LR1121 en este modelo Base Uno/SX1262.

## Alcance

- Batería Bluetooth BAS con reintentos y entrega al suscribirse.
- Corrección del aviso falso del cargador BQ25185.
- Bluetooth controlable desde la pantalla y preferencias persistentes.
- Modos de buzzer All, Off, Messages, System y DMs Only.
- Seis controles con captura de flancos, filtrado de rebotes y sondeo alternativo.
- Atrás para apagar la pantalla y centro para despertarla.
- Página LEDs con preferencia On/Off persistente.
- GPS controlado exclusivamente por el interruptor físico: Mode 1 apagado,
  Mode 2 encendido. Su página es informativa.
- Reposo de sensores sin uso en Companion y preparación de apagado.
- Guardado de preferencias mediante archivo temporal y recuperación RX de radio.
- Actualización Bluetooth con arranque automático y conservación del vínculo,
  usando el cargador compatible indicado más abajo.
- Emojis monocromos en nombres, contactos y mensajes: 3.953 secuencias OpenMoji.

Atrás para apagar pantalla y el control de LEDs se añadieron por solicitud
expresa durante la validación. Se conservan el protocolo MeshCore y los datos
guardados del usuario. El alcance no incluye teclado autónomo ni brújula.

## Uso de los controles

Con la pantalla apagada, una pulsación central la enciende y consume esa entrada.
Con la pantalla encendida, centro ejecuta la acción de la página actual y las
direcciones recorren las páginas.

Una pulsación corta de **Atrás apaga la pantalla** y, si ya está apagada, la deja
así. La pulsación simple espera la ventana de reconocimiento de múltiples clics,
aproximadamente 280 ms. Triple Atrás conserva el cambio de modo de buzzer. La
pulsación larga conserva su función existente.

La página **LEDs**, junto a sonido, muestra On/Off. Centro cambia el estado y lo
guarda. On recupera el parpadeo verde normal; Off lo suprime. La preferencia se
restaura tras reiniciar. Bluetooth y sonido tienen sus propias páginas y
preferencias; apagar los LEDs no apaga Bluetooth ni la radio.

### Límites del control de LEDs

La opción controla los indicadores MCU durante la aplicación: verde en
P1.03/GPIO 35 y azul en P1.04/GPIO 36. El azul se mantiene apagado conforme a la
configuración previa de este fork; On no introduce otra señalización azul.

El rojo de carga D4 depende de la red `BAT_CHG_STATUS`, conectada a STAT2 del
BQ25185. Continúa indicando carga por hardware y queda fuera de esta preferencia.
Lo confirma el [esquema oficial, página 1](https://cdn.shopify.com/s/files/1/0657/6973/4201/files/Base_Duo_Schematic_Rev01.pdf?v=1766439439),
en los bloques LEDS y BATTERY CHARGER. Los destellos del cargador de arranque al
encender, recuperar o actualizar el nodo también quedan fuera del control de la
aplicación. La función LEDs no modifica el cargador de arranque ni la carga de
la batería.

## Estado de validación

La [matriz de implementación y pruebas](docs/superbase-rc1-validation.md) distingue
el instalable exacto, pruebas de código, evidencia física y pendientes. El ZIP de
producción ensayado tiene SHA-256
`1f0e400df7e9b6052608aed4380e55fe3183d0b6aba55f85cf15a4445b87d943`.
La compilación es optimizada, sin interfaz Companion USB de diagnóstico ni trazas.
Las imágenes debug y los registros internos no se distribuirán.

Comprobaciones físicas de esta imagen:

- Seis controles, Atrás para apagar pantalla y centro para despertarla: confirmados
  por el usuario. El falso aviso del cargador está ausente.
- LEDs On y Off persisten tras RESET; On parpadea en verde y Off lo detiene. Off
  también se conservó durante la actualización Bluetooth.
- Bluetooth Off y buzzer Off persistieron tras RESET; Bluetooth volvió a On desde
  el nodo y se pudo conectar de nuevo. Se vieron los cinco nombres de modo de
  buzzer en pantalla; los filtros de los tres modos selectivos no tienen todavía
  un ensayo físico completo.
- Un ciclo OTA con Nordic predeterminado y cargador `v2.3-muzi.2-rc1` transfirió,
  validó y arrancó automáticamente. USB regresó en 49,24 s desde su desaparición
  y MeshCore reconectó con el mismo vínculo: último BONDED `00:07:18.398`
  idéntico antes y después, sin PIN nuevo, RESET, reconectar el cable ni modificar
  ajustes del teléfono.
- Mensajes directos SuperBase → Heltec V4 y V4 → SuperBase: Delivered y observados
  en ambos receptores. Con buzzer Off y LEDs Off, el usuario confirmó silencio y
  ausencia de destello verde. Otra respuesta recibida con buzzer All quedó
  Delivered y produjo aviso audible; el verde siguió apagado.

El porcentaje de batería visible en la app Android actual no sustituye la
comprobación del servicio GATT BAS: las lecturas/notificaciones BAS documentadas
se obtuvieron con imágenes internas anteriores y falta repetirlas en esta imagen.

La captura por interrupciones se comprobó en una imagen interna anterior: los
seis controles produjeron flancos y no hubo descartes en esa muestra. La prueba
funcional actual confirma sus acciones; no garantiza toda pulsación muy breve
durante cualquier operación bloqueante.

Siguen pendientes GPS exterior y arranque/sesión a batería sobre el instalable
actual de la corrección del GPS. En la imagen anterior `1f0e400…`, el usuario
confirmó fix con ocho satélites a batería, apagado en Mode 1 y nueva adquisición
en Mode 2. A petición del usuario, la fuente ahora elimina el control de GPS por
software y su dependencia de la preferencia guardada. Esta corrección requiere
su propia compilación, instalación y prueba; los resultados anteriores no se
transfieren automáticamente. Quedan además las comprobaciones
específicas identificadas en la matriz, como GATT BAS y filtros selectivos del buzzer.
No hay medición instrumental de consumo, descarga completa, alcance máximo ni
ensayos eléctricos inducidos. El ciclo OTA con USB no aprueba autonomía ni otras
combinaciones de teléfono y cargador. Al cierre del 28 de octubre de 2026, la
candidata sigue sin aprobar como estable y la publicación permanece detenida.

## Archivos previstos e instalación

Estos son los nombres previstos para la candidata, todavía sin publicar:

- `MeshCore-Muzi-Works-SuperBase-Companion-BLE-v1.18-sb2-rc1.uf2`: aplicación UF2.
- `MeshCore-Muzi-Works-SuperBase-Companion-BLE-v1.18-sb2-rc1-dfu.zip`: aplicación
  para Nordic DFU; seleccionar directamente este ZIP al actualizar.
- `Muzi-Works-SuperBase-Bootloader-v2.3-muzi.2-rc1.uf2`: cargador compatible para
  la conservación del vínculo durante OTA.
- `SHA256SUMS.txt`, `BUILD.json` y `VALIDATION.json`: integridad, fuente y auditoría.

Los dos instalables de aplicación no contienen cargador ni SoftDevice. El UF2
del cargador actualiza ese componente y conserva S140 6.1.1; no incluye la
aplicación. Son archivos distintos y no son intercambiables.

### Requisito del cargador

Para el flujo OTA con vínculo conservado se requiere **v2.3-muzi.2-rc1**. El UF2
compatible ensayado tiene SHA-256
`5d8163cc5a4f28bef2639388c83a95c1b846ab4b8a2222efe963d8d0eda35114`.
El cargador anterior `v2.3-muzi.1` no tiene aprobado ese flujo. La firma de la
rutina de intercambio del cargador se reconoce de forma exacta en la aplicación;
el nombre de versión por sí solo no acredita compatibilidad con otra compilación.

Para instalar por USB, conectar el nodo, entrar al cargador con doble RESET y
comprobar `Board-ID: muzi-Base-Board` en `INFO_UF2.TXT`. Si corresponde actualizar
el cargador, copiar su UF2 a esa unidad y esperar a que termine. Después, volver
al modo de recuperación USB cuando sea necesario e instalar el UF2 de aplicación.
Con el cargador compatible ya instalado, basta instalar o actualizar la aplicación.

Para OTA, usar la app Nordic existente en sus valores predeterminados y seleccionar
el ZIP de aplicación. No se requieren nuevas apps ni cambios de configuración del
teléfono. El emparejamiento inicial de MeshCore utiliza el PIN de la pantalla;
el valor alternativo de compilación `123456` puede diferir del PIN activo.
**Una solicitud de volver a emparejar después de OTA se registra como fallo de
conservación del vínculo.** No se indica borrar el emparejamiento ni introducir
otro PIN como parte normal de esta actualización.

El interruptor físico controla el GPS: Mode 1 lo apaga y Mode 2 lo enciende,
también al arrancar. La página GPS muestra estado y satélites; pulsar centro no
cambia su alimentación. Las solicitudes de encendido/apagado por software se
rechazan; la antigua preferencia GPS se conserva para compatibilidad de datos
y se ignora como control. La recepción dentro de edificios no está garantizada.
La radio conserva los parámetros guardados.
El valor de compilación para nodos sin configurar sigue siendo 869.618 MHz;
este fork no selecciona una región.

## Compilar y verificar

```text
pio run -e muzi_base_uno_superIO_companion_radio_ble -t buildprog -t create_uf2
python test/superbase/run.py
```

Linux/WSL también admite `python test/superbase/run.py --sanitize`. El flujo
preparado **Build SuperBase Companion BLE** compila el perfil de producción,
ejecuta regresiones y audita los límites de flash/RAM y la coincidencia de
aplicación UF2/DFU.

GitHub Actions no ejecutó ese flujo nuevo, que todavía no existe en la rama
predeterminada del repositorio. La compilación ARM y la auditoría de estos
instalables se realizaron localmente; las regresiones se ejecutaron en Windows
y Linux/WSL. No se atribuye a esta candidata un resultado de CI que no ocurrió.

El soporte de hardware procede de MeshCore oficial, base
`81dbae592ffad40b4212bc0b37e82b38da08f4bc`. Las mejoras adaptadas se basan en
el alcance acordado del fork de Meshtastic, sin trasladar mecanismos de MQTT ni
sus cambios de seguridad específicos. La [atribución OpenMoji](src/helpers/ui/emoji/README.md)
y su licencia CC BY-SA 4.0 acompañan los pictogramas.
