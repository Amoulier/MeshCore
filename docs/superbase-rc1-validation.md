# Muzi Works SuperBase: revisión de alcance de RC1

Estado actualizado: **28 de octubre de 2026, America/La_Paz**.
Versión: **v1.18-sb2-rc1**. Hardware: **Base Uno nRF52840/SX1262 + Super IO**.
Uso: Companion MeshCore por Bluetooth.

## Estado de la candidata

**Validación funcional de la candidata completada dentro del alcance descrito.**
OTA con vínculo conservado, BAS nativo Windows, modos selectivos del buzzer y
sesión continua de 30 minutos a batería pasaron en el instalable final
`637f60…`. Se distribuye como candidata RC1; no se declara una versión estable
general ni se amplían esos resultados a prestaciones no medidas.

| Identificación | Valor |
| --- | --- |
| ZIP de aplicación instalado | `637f60b169b55d7fa44e107a20943841db0354b34793acf6b3ea0c133dd42f4b` |
| UF2 de aplicación | `7601f365ad1bf626cc725eb2939e9c9250aecf00dd2605846fb6c3abaa508434` |
| Código compilado | `e8d3ef4d3f31711a857f8925a2e9435b3a868047` |
| Aplicación | 452.152 bytes; aplicación coincidente en UF2 y DFU |
| Cargador compatible instalado | `v2.3-muzi.2-rc1`, S140 6.1.1 |
| SHA-256 del UF2 del cargador | `5d8163cc5a4f28bef2639388c83a95c1b846ab4b8a2222efe963d8d0eda35114` |
| Perfil | `muzi_base_uno_superIO_companion_radio_ble`, optimización de producción |
| Diagnósticos y trazas internas | Excluidos de los instalables previstos |

La compilación ARM y la auditoría del paquete pasan. Las regresiones del código
funcional pasaron en Windows y en Linux/WSL con ASan/UBSan. La evidencia de compilación y regresiones de estos instalables es local.
Los resultados de CI se identifican por separado en los metadatos de la entrega;
un artefacto posterior de CI no sustituye automáticamente los bytes ensayados.
Una auditoría estática no certifica el hardware.

## Pruebas físicas del instalable final `637f60…`

### Actualización Bluetooth y conservación del vínculo

**PASS:** Nordic en sus valores predeterminados transfirió los **452.152 bytes**,
validó y activó la aplicación. USB regresó automáticamente en **49,27 s** desde
su desaparición. La primera reconexión normal de MeshCore quedó **cifrada y
vinculada**, sin PIN nuevo ni eventos nuevos de emparejamiento, sin RESET manual
y sin desconectar/reconectar USB. No se cambiaron los ajustes de Nordic.

La evidencia de este ciclo es la conexión segura/vinculada y la ausencia de
nuevos eventos de emparejamiento. **No se compara contra una antigua marca
BONDED:** esa entrada ya había salido del historial disponible. Tampoco se
convierte el resultado en una aprobación de todos los teléfonos o cargadores.

### Servicio de batería Bluetooth BAS

**PASS con cliente nativo Windows:** se utilizaron el adaptador Bluetooth ya
encendido y el vínculo existente, sin pedir otro emparejamiento ni modificar
ajustes del teléfono o de la computadora. Se completaron **tres reconexiones,
seis lecturas y seis notificaciones reales**, con nivel observado de 84%.
Después de desuscribirse se observaron **cero notificaciones en las seis
ventanas de silencio**. Los estados de los adaptadores permanecieron intactos.
Esto valida lectura, suscripción, parada y nueva suscripción en ese recorrido;
no calibra la batería ni mide autonomía.

**Chrome Android: recorrido incompleto.** En las primeras conexiones se pudieron
leer BAS, recibir una notificación y detenerla; las ventanas posteriores a la
parada quedaron sin notificaciones. En **dos intentos**, la lectura posterior a
reconectar terminó por tiempo de espera. La limitación está observada en ese
recorrido de cliente/validación; no se ha establecido su causa. El contraste
nativo Windows pasó con el mismo firmware. No se atribuye el tiempo de espera
al firmware ni se declara aprobada la compatibilidad de reconexión de Chrome.
Los permisos de Dispositivos cercanos y del sitio usados en esa prueba fueron
expresamente autorizados; no se alteraron otros ajustes de las aplicaciones.

### Modos selectivos del buzzer

**PASS físico en `637f60…`:** los tres modos conservaron su selección tras RESET.
El usuario confirmó los resultados siguientes; cada DM y texto de canal se
observó en el SuperBase antes de aprobar una recepción silenciosa.

| Modo | Melodía de arranque | DM del Heltec V4 | Texto del canal privado |
| --- | --- | --- | --- |
| Messages | Silencio | Audible | Audible |
| System | Audible | Silencio | Silencio |
| DMs Only | Silencio | Audible | Silencio |

Se usó el canal privado temporal `SB-RC1-TEST`, únicamente entre los dos nodos
propios y con autorización. **La limpieza ya está verificada en ambos nodos:**
se retiró el canal de prueba y se conservaron las entradas previas; el Heltec
volvió a sus seis canales originales. No se leyeron ni exportaron claves
existentes. El usuario confirmó al terminar **Buzzer All y LEDs Off**.

Los textos de canal no tienen confirmación de entrega equivalente a un DM:
la prueba se apoya en su lectura en el receptor y en la observación del sonido.
Los modos Off y All tenían además comprobaciones físicas anteriores de silencio
y aviso con DMs, identificadas abajo. No se presentan como ensayos nuevos de
cada tipo posible de evento del sistema.

### Sesión continua a batería

**PASS:** sesión continua de 30 minutos a batería en el instalable final, con 61 lecturas válidas, 44 notificaciones reales y cero desconexiones inesperadas. USB permaneció ausente, no se reconectó automáticamente y no hubo RESET. La conexión de prueba se cerró correctamente; los estados Bluetooth de Windows permanecieron intactos. El nivel observado osciló entre 82% y 83%. Es una comprobación de continuidad, no una medida de autonomía total.

### Pantallas, iconos e instalación USB

**PASS visual y funcional:** el usuario eligió y confirmó en el nodo ambos
estados de las páginas LEDs/Buzzer. Son bitmaps de 32 × 32 derivados de
Google Material Icons 3.0.1, Apache 2.0, con una sola leyenda en y=53 y los
Bluetooth originales conservados. Los SVG, modificaciones y atribuciones están
en el [README del artwork](../examples/companion_radio/ui-new/icons/material/README.md).
La instalación USB y el retorno automático al firmware también pasaron.

## Implementación y ámbito de la evidencia conservada

Los cambios finales de iconos son de presentación; no modificaron el código de
GPS, radio ni las políticas funcionales. La evidencia siguiente conserva la
identificación de la imagen realmente ensayada. No se presenta una observación
anterior como medición nueva en `637f60…`.

La corrección de GPS físico se ensayó en el ZIP
`13317e834cd4835b58febddcc82d2fca8ac0700d77c635b52af3423615c8230f`,
código `949296ce97a79d5fa0b99deafe17ce8e9f869437`. La evidencia anterior de
preferencias/controles corresponde al ZIP
`1f0e400df7e9b6052608aed4380e55fe3183d0b6aba55f85cf15a4445b87d943`.

| Mejora | Implementación y evidencia | Límite práctico |
| --- | --- | --- |
| GPS exclusivamente físico | En `13317e…`, Mode 1 mostró Off/no fix y Mode 2 On; centro no alteró el GPS. A batería, RESET en cada modo respetó el interruptor; en exterior recuperó fix con ocho satélites. Se rechazan escrituras GPS por software y se ignora la preferencia heredada como puerta. Los métodos reales pasan regresiones y sanitizadores. | Esa adquisición y esos arranques se atribuyen a `13317e…`; no se reabren por cambiar solo iconos. La sesión continua final pasó; no mide autonomía total. No se promete recepción interior. |
| Radio SX1262 | En `13317e…`, SuperBase → Heltec V4 (`RC1-GPS-SB-V4-01`) y V4 → SuperBase (`RC1-GPS-V4-SB-01`) quedaron Delivered y se observaron en ambos receptores sin RESET antes de leer. En `637f60…` se recibieron los DMs y textos de canal de las pruebas del buzzer. Recuperación RX/protección TX cubiertas con fallos simulados. | La prueba bidireccional se mantiene en su ámbito de `13317e…`; no se alteraron parámetros de radio. Alcance máximo y fallo físico inducido sin medir. |
| Seis controles y Atrás | En `1f0e400…` se confirmaron las seis acciones; en `13317e…`, Atrás apaga, centro despierta y la página GPS es informativa. La captura interna previa registró máscara 63, dos flancos adicionales por control y cero descartes; se mantiene sondeo alternativo. | No garantiza toda pulsación breve durante cualquier operación bloqueante. Triple Atrás y pulsación larga conservan sus funciones. |
| LEDs | Verde GPIO 35 y azul GPIO 36 sujetos a la preferencia persistente. En `1f0e400…`, On/Off persistieron tras RESET y Off tras OTA, sin destello verde durante DMs. En `637f60…` se aprobaron visualmente los dos estados y se confirmó finalizar Off. | No se convierte la persistencia histórica en un nuevo ensayo de todas las combinaciones. Rojo de carga y destellos del cargador quedan fuera. |
| Bluetooth desde pantalla | En `1f0e400…`, Off persistió tras RESET y On permitió reconectar. En `637f60…` pasó la primera reconexión segura/vinculada tras OTA. | La OTA actual no es por sí sola otra prueba de Off/RESET/On ni de todos los teléfonos. |
| Buzzer Off/All y modos selectivos | Off/All con DMs y persistencia de Off se comprobaron en `1f0e400…`. La tabla física de Messages, System y DMs Only y su persistencia tras RESET ya pasó en `637f60…`. | Se probaron arranque, DM y texto de canal; no cada evento imaginable. |
| Cargador BQ25185 | STAT1 con pull-up y distinción carga/reposo/fallo; regresiones. La desaparición de `! Check charger` fue confirmada en `1f0e400…`. | El control del cargador no cambió con los iconos; corriente de carga y fallos eléctricos inducidos no medidos. |
| Preferencias y vínculo | Temporal + rename, reintentos y vaciado previo; fallos de almacenamiento simulados. Guardado CCCD probado con longitudes distintas y campos de claves/nombre sintéticos protegidos. En `637f60…`, los tres modos selectivos persistieron y la OTA reconectó vinculada. | Corte real de alimentación durante escritura sin ensayar; contactos heredados no informan todos sus errores y no hay transacción conjunta. |
| Energía y sensores | Reposo IMU/magnetómetro y parada de periféricos observados en imágenes internas; arranques a batería en ambos modos GPS confirmados en `13317e…`. | Sesión continua final aprobada; no mide autonomía total. Consumo instrumental y descarga completa sin medir. |
| Emojis | UTF-8 y 3.953 secuencias OpenMoji 17 con recorte/saltos seguros, catálogo y límites con sanitizadores; joystick del nombre confirmado en `13317e…`. | Monocromo de 10 × 10; no se ensayaron físicamente los 3.953 pictogramas ni se conserva el detalle/color original. |
| RTC | Se conserva el reloj heredado; lectura y avance RX8130CE observados en una imagen interna. | Precisión y retención no medidas; no se añadió otra integración de hora. |

## Cargador compatible y límites de LEDs

La OTA con vínculo conservado requiere el cargador **v2.3-muzi.2-rc1** ensayado.
Se instaló por USB con autorización y `INFO_UF2.TXT` confirmó versión y S140 6.1.1.
Corrige el inicio del aviso Service Changed mediante el handle real de DFU;
la aplicación reconoce su intercambio, prepara USB y persiste correctamente los
atributos de suscripción. El paquete global del framework no se modifica.
El cargador anterior `v2.3-muzi.1` no tiene aprobado este flujo.
El ciclo final `637f60…` documentado arriba es la prueba actual de la pareja.

El UF2 del cargador contiene ese componente con su configuración Muzi; no
incluye la aplicación ni reemplaza SoftDevice. Los instalables de aplicación
no contienen cargador ni SoftDevice. No son imágenes debug.

La preferencia LEDs actúa durante MeshCore. El rojo D4, a través de R22, depende
de `BAT_CHG_STATUS`/STAT2 del BQ25185 y queda fuera de ella, como muestra el
[esquema oficial, página 1](https://cdn.shopify.com/s/files/1/0657/6973/4201/files/Base_Duo_Schematic_Rev01.pdf?v=1766439439).
El fabricante indica el pinout compartido Uno/Duo en sus
[archivos de referencia](https://muzi.works/pages/base-system-downloads).
Los destellos del cargador de arranque también quedan fuera de esta preferencia.

## Cierre y límites

Las validaciones funcionales pendientes quedaron completadas. La dependencia del
cargador, los hashes, la fuente y las licencias acompañan la distribución.
El recorrido de reconexión Chrome Android continúa sin aprobar: se documenta
como limitación observada del cliente de prueba, sin causa atribuida. BAS nativo
Windows y el flujo normal de MeshCore sí tienen los resultados descritos.

No se han medido consumo instrumental, descarga completa, alcance máximo,
precisión/retención RTC ni fallos eléctricos o cortes durante escritura.
No se prometen esas prestaciones a partir de las pruebas funcionales.

No forman parte del alcance: ACK por MQTT, cambios de seguridad específicos de
Meshtastic, teclado autónomo, brújula ni temporizador estacionario de 12 horas.
No se crearon respaldos ni se exportaron identidad, claves o configuración del
nodo. La prueba privada temporal ya se retiró; los registros internos y datos
del nodo quedan fuera de la publicación. Se conserva el protocolo MeshCore.
