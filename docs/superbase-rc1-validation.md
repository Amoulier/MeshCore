# Muzi Works SuperBase: revisión de alcance de RC1

Revisión de alcance iniciada: 27 de octubre de 2026, America/La_Paz.
Estado al cierre: 28 de octubre de 2026, America/La_Paz.
Versión: **v1.18-sb2-rc1**. Hardware: **Base Uno nRF52840/SX1262 + Super IO**.
Uso: Companion MeshCore por Bluetooth.

## Estado de la candidata

**Publicación detenida. No hay una candidata aprobada como estable.**
El alcance incluye las mejoras acordadas y dos solicitudes expresas durante la
validación: pulsación corta de Atrás para apagar la pantalla y una opción
persistente de LEDs en la interfaz. El perfil normal usa optimización de
producción, sin comandos USB de diagnóstico ni trazas.
Los binarios internos, datos del nodo y registros de desarrollo no se publican.

**Corrección en preparación solicitada el 28 de octubre:** el GPS debe obedecer
exclusivamente al interruptor físico (Mode 1 Off, Mode 2 On), también al arrancar.
La página pasa a ser informativa y las escrituras de activación GPS por Companion
se rechazan sin cambiar estado ni preferencias. La antigua preferencia guardada
deja de impedir el arranque GPS en Mode 2. El receptor integrado ya no recibe un
pulso de detección al arrancar en Mode 1. Esta corrección necesita instalación y
verificación propias; los ensayos del ZIP `1f0e400…` siguen identificados como
evidencia anterior, no como aprobación automática del nuevo binario.

Los resultados físicos siguientes no se transfieren automáticamente a cada
recompilación. Se identifica qué implementación se ensayó y qué falta repetir
sobre el instalable final. Compilación y auditoría no certifican el hardware.

El instalable de aplicación actual es el ZIP de producción con SHA-256
`1f0e400df7e9b6052608aed4380e55fe3183d0b6aba55f85cf15a4445b87d943`,
compilado desde la revisión de código `4a7577d37612becba55ebc431ea71313ada2b999`.
El UF2 correspondiente tiene SHA-256
`54739510b7c3b1cfa088b03d6312fe20f9b25fc7e9e8119750ea2146d0754028`.
Incluye Atrás para apagar pantalla y la nueva página de LEDs. Compilación ARM y
auditoría del paquete pasan. En este archivo exacto quedaron comprobados los
seis controles, Atrás para apagar pantalla y centro para despertarla, ausencia
del aviso falso del cargador, LEDs On/Off persistentes tras RESET, Bluetooth Off
y buzzer Off persistentes tras RESET y posterior activación de Bluetooth.

**La OTA del archivo actual pasó:** con Nordic predeterminado y cargador
`v2.3-muzi.2-rc1`, USB regresó automáticamente en **49,24 s**. MeshCore reconectó
con el mismo vínculo, cuyo último evento BONDED seguía en **00:07:18.398**, sin
nuevo PIN, RESET, reconectar USB ni cambiar ajustes del teléfono. LEDs Off se
conservó durante esa OTA. El cargador ensayado tiene SHA-256
`5d8163cc5a4f28bef2639388c83a95c1b846ab4b8a2222efe963d8d0eda35114`.

También pasó radio bidireccional con Heltec V4: SuperBase → V4 y la respuesta
V4 → SuperBase quedaron Delivered y se observaron en cada receptor. Con buzzer
Off y LEDs Off, el usuario confirmó recepción silenciosa y sin destello verde.
Con buzzer All, una respuesta recibida quedó Delivered y el usuario oyó el aviso;
el verde permaneció apagado. Los cinco nombres de modo de buzzer se recorrieron
en pantalla; esto no equivale a ensayar físicamente cada filtro de los tres
modos selectivos.

**GPS exterior y sesión a batería siguen pendientes sobre este instalable.**
El 28 de octubre, con el ZIP `1f0e400…`, el usuario confirmó funcionamiento
exclusivamente a batería, fix con ocho satélites, apagado/no fix en Mode 1 y
recuperación de fix al volver a Mode 2. MeshCore conectó por Bluetooth a la RC1.
La consola Companion reconoce versión y placa, pero
no ofrece las consultas `power`, `gps diag` ni `sensor get gps` de CommonCLI;
sus respuestas `Unknown command` no acreditan un fallo del GPS. No se cambió
el firmware para añadir diagnósticos. No se ejecutó la prueba de apagado GPS
desde el menú: el usuario aclaró que ese control debe ser exclusivamente físico.
Se conserva la candidata sin publicar y
no se declara aprobada como estable.

Se conserva como evidencia técnica el ciclo OTA anterior del ZIP de producción
`ec7f774c2ac15d2d79518b49a2b2ee51bf0ad09b35e47185528d8670f7bc7e50`
(código `12b00de0a8229c2d8c4d00c872eed159978e5fd9`). Con Nordic predeterminado y
cargador `v2.3-muzi.2-rc1`, ese archivo transfirió y validó; USB regresó
automáticamente en 49,44 s y MeshCore reconectó con el mismo vínculo, sin nuevo
PIN, RESET, reconectar el cable ni cambiar ajustes del teléfono. Ese ciclo
anterior es evidencia histórica; la aprobación del archivo actual corresponde
al ensayo de 49,24 s descrito arriba. Ambos se hicieron con USB conectado y no
acreditan autonomía ni las demás funciones pendientes.

GitHub Actions no ejecutó el flujo nuevo porque aún no existe en la rama
predeterminada. Las compilaciones ARM, regresiones Windows y sanitizadores
Linux/WSL se realizaron localmente. No se atribuye aprobación de CI.
Las correcciones de suscripciones Bluetooth descritas abajo pasaron las
regresiones nativas, AddressSanitizer/UndefinedBehaviorSanitizer y la compilación
ARM. La comparación del vínculo antes y después del ciclo OTA actual confirmó
el mismo evento de emparejamiento y conexión MeshCore sin una nueva solicitud.

## Implementación y verificación

| Mejora acordada | Implementación y evidencia | Validación pendiente |
| --- | --- | --- |
| Batería Bluetooth | BAS 0–100, reintentos y nueva suscripción; regresiones de entrega. La lectura/notificación GATT BAS en Windows corresponde a imágenes internas anteriores. El porcentaje visible en la app Android actual no demuestra por sí solo esa lectura/notificación BAS. | Repetir la comprobación GATT BAS sobre el instalable final. Calibración si se promete autonomía. |
| Cargador BQ25185 | STAT1 con pull-up y distinción carga/reposo/fallo. Usuario confirmó desaparición de `! Check charger`; entradas y fallos simulados cubiertos. | Fallos eléctricos inducidos y corriente de carga sin medir. |
| Bluetooth desde pantalla | Habilitación en ejecución y preferencia persistente. En `1f0e400…`, Bluetooth Off persistió tras RESET; el usuario lo volvió a On y MeshCore conectó. La reconexión después de OTA conservó el vínculo. | No queda pendiente ese recorrido Off/RESET/On; no se extrapola a todos los teléfonos. |
| Buzzer | Modos All, Off, Messages, System y DMs Only; regresiones de políticas y propiedad. En `1f0e400…`, el usuario vio los cinco nombres y Off persistió tras RESET. Un DM confirmado Delivered y visto en SuperBase fue silencioso en Off; otro DM confirmado Delivered produjo aviso audible en All. | Ensayo físico de los filtros Messages, System y DMs Only; sus nombres en pantalla no prueban su selección de avisos. |
| Seis controles y Atrás para apagar pantalla | Captura de flancos, rebotes, pulsación larga, triple Atrás y sondeo alternativo; regresiones. En `1f0e400…`, el usuario confirmó los seis controles, Atrás para apagar y centro para despertar. El CLICK corto de Atrás se consume y no despierta una pantalla ya apagada. Triple Atrás y pulsación larga se conservan. Captura interna previa: máscara 63, dos flancos adicionales por control, cero descartes. | No queda pendiente la confirmación de las seis acciones actuales; no se afirma captura infalible de toda pulsación breve durante cualquier operación bloqueante. |
| LEDs desde la pantalla, solicitud nueva | Página LEDs junto a sonido, centro alterna On/Off y guarda `comp.led_en`, predeterminado On. Verde P1.03/GPIO 35; azul P1.04/GPIO 36 se mantiene apagado. En `1f0e400…`, On restauró el parpadeo verde y persistió tras RESET; Off lo detuvo, persistió tras RESET y sobrevivió a OTA. Durante los DMs silencioso y audible, el usuario confirmó ausencia de destello verde con LEDs Off. | Recorridos On/Off y persistencia aprobados. Rojo de carga y destellos del cargador de arranque quedan fuera; véanse los límites de hardware. |
| GPS | Fuente corregida para control exclusivamente físico, arranque según Mode 1/2, rechazo de escrituras GPS y página informativa. Se conserva descarte de fix al apagar/iniciar. En el ZIP anterior `1f0e400…`, el usuario confirmó ocho satélites, fix a batería y apagado/nueva adquisición por switch. | Compilar, instalar y comprobar la corrección física exclusiva: centro no cambia GPS, ambos modos y arranque a batería en cada modo. No se promete recepción interior. |
| Energía y apagado | Reposo IMU/magnetómetro, parada de buzzer/pantalla/GPS y arranque a batería observados en imágenes internas. En `1f0e400…`, el usuario confirmó alimentación exclusivamente a batería y se observaron respuestas Bluetooth. | Arranque y sesión a batería en el nuevo instalable. Sesión prolongada, descarga completa y consumo instrumental sin medir. |
| Preferencias | Temporal + rename, reintento y vaciado previo; pruebas de apertura, escritura incompleta, borrado y rename fallidos. En `1f0e400…`, LEDs On/Off, Bluetooth Off y buzzer Off persistieron tras RESET; LEDs Off también durante OTA. Radio e identidad se conservaron. La preferencia GPS heredada se conserva por compatibilidad, pero se ignora como puerta en SuperBase. | Verificar arranque según el switch en el nuevo binario; corte de alimentación durante escritura sin ensayar. Contactos heredados no informan todos sus errores; no hay transacción conjunta. |
| Radio SX1262 | Recuperación RX y protección TX con fallos simulados. En el instalable actual `1f0e400…`, SuperBase → Heltec V4 y V4 → SuperBase quedaron Delivered y se vieron en el receptor; se comprobó recepción con buzzer Off y con All, conservando LEDs Off. No se modificaron parámetros de radio. | Mensajería bidireccional actual aprobada; alcance máximo y fallo físico inducido sin ensayar. |
| Actualización Bluetooth | Guardado previo, corrección de USB y RAM de intercambio, autorización S140 y entrada cifrada con vínculo compartido. Ambos arreglos CCCD pasan pruebas nativas, sanitizadores y ARM. **Ciclo actual aprobado:** ZIP `1f0e400…`, cargador `v2.3-muzi.2-rc1` SHA `5d8163…`, Nordic predeterminado, USB automático en 49,24 s y reconexión con el mismo último BONDED `00:07:18.398`, sin nuevo PIN, RESET, reconectar USB ni cambiar ajustes del teléfono. LEDs Off sobrevivió. | Un ciclo completo con USB aprobado para esta pareja exacta de app/cargador. No acredita autonomía ni otras combinaciones de cargador/teléfono. Una nueva petición de emparejamiento sería un fallo, no un paso normal. |
| Emojis | UTF-8, 3.953 secuencias OpenMoji 17, recorte y saltos sin partir secuencias. Catálogo y límites con sanitizadores. Usuario confirmó joystick en nombre real y pantalla RC1 actual. | Confirmación visual del joystick aprobada; no se ensayaron físicamente los 3.953 pictogramas. Monocromo de 10 × 10: no conserva color ni todo el detalle. |
| RTC | Se conserva el reloj heredado. Lectura y avance de segundos RX8130CE observados en imagen interna. | Precisión y retención no medidas; no se añade otra integración de hora. |

## Fallos y efecto real

- **Aviso falso de cargador:** corrección observada y cubierta por regresiones.
- **Bloques del emoji:** corrección observada; se conserva el nombre UTF-8.
- **Radio sin confirmación inicial:** resuelto tras intercambiar anuncios; no
  se reprodujo como fallo persistente. El instalable actual tuvo comunicación
  bidireccional con Heltec V4, Delivered y observada en ambos receptores.
- **Captura por interrupciones:** los contadores a cero de una prueba anterior
  no describen la última evidencia. Los seis controles produjeron flancos reales
  antes de instalar RC1. Se mantiene el sondeo alternativo.
- **Pantalla negra:** el apagado por inactividad se distingue del bloqueo DFU.
  El usuario confirmó retorno automático y despertar con centro en el instalable
  actual, sin RESET ni reconectar USB durante la OTA.
- **Atrás sin efecto visible:** se detectó que volvía a la página inicial.
  A petición explícita se cambió a apagar la pantalla con una pulsación corta.
  El usuario confirmó apagado con Atrás y despertar con centro en el archivo
  actual con LEDs. El temporizador y el renderizado no la reencienden; la política
  previa de despertar por mensajes entrantes se conserva.
- **Pérdida de emparejamiento tras OTA:** fue un fallo real del flujo y bloqueó
  su aprobación. Nordic Legacy elimina el vínculo con sus valores predeterminados.
  La entrada con vínculo compartido evita pedir ese borrado, pero exige que el
  cargador y la aplicación anuncien correctamente sus cambios de servicios.
  Un emparejamiento manual completado por el usuario después de un fallo anterior
  establece la base para el nuevo ensayo; no se cuenta como conservación del
  vínculo durante aquel fallo. **El ciclo OTA actual de `1f0e400…` conservó
  ese vínculo y reconectó MeshCore sin volver a emparejar:** último BONDED
  `00:07:18.398` idéntico antes y después. Una petición de PIN nuevo tras OTA
  se considera fallo de conservación del vínculo.
- **Caché GATT al entrar al cargador:** reproducida en dos secuencias, sin
  transferencia de firmware y con `GATT INVALID HANDLE`. La corrección del
  cargador permitió después una transferencia interna de 451.512 bytes,
  validación y retorno automático de USB en 49,35 s. Ese ensayo utilizó otra
  imagen y reveló un problema posterior de reconexión; no aprobó todo el flujo.
- **Caché GATT al volver a MeshCore:** aunque Android conservaba el vínculo,
  seguía descubriendo los servicios del cargador. Se identificaron dos fallos de
  gestión CCCD. Primero, el descriptor de sistema de Service Changed no se
  persistía y el aviso no se reintentaba cuando el teléfono lo habilitaba después
  de asegurar el enlace. Segundo, el guardado del campo CCCD escribía desde una
  posición incorrecta cuando cambiaba su longitud.
  La aplicación ahora persiste ese descriptor de sistema y reintenta el aviso al
  habilitarlo en el enlace actual cifrado y vinculado, evitando avisos extra
  después de un envío aceptado o durante la transición DFU. La copia local de
  Bluefruit guarda el inicio del campo y comprueba el reposicionamiento antes de
  escribir. Las pruebas con datos sintéticos cubren longitudes mayor/menor,
  valores cambiados/idénticos, campo ausente, conservación de claves/nombre y
  fallo de reposicionamiento sin escritura. El paquete global del framework no
  se modifica. **La reconexión tras OTA del ZIP actual `1f0e400…` pasó con el
  mismo vínculo y sin otro PIN.** La evidencia anterior de `ec7f774…` se conserva
  como antecedente y no sustituye el ensayo actual.

## Límites del control de LEDs

La preferencia actúa sobre GPIO 35/36 durante la aplicación. El LED rojo D4,
mediante R22, está conectado a `BAT_CHG_STATUS`, red que proviene de STAT2 del
BQ25185: es un indicador del cargador de batería, no un LED MCU independiente.
La opción no modifica esa salida ni el proceso de carga. El
[esquema oficial, página 1](https://cdn.shopify.com/s/files/1/0657/6973/4201/files/Base_Duo_Schematic_Rev01.pdf?v=1766439439)
lo muestra en los bloques LEDS y BATTERY CHARGER; el fabricante indica el mismo
pinout para Uno y Duo en sus
[archivos de referencia](https://muzi.works/pages/base-system-downloads).

La preferencia se carga al arrancar MeshCore. Los destellos del cargador de
arranque durante arranque, recuperación o actualización quedan fuera de este
control. Ese cargador permanece sin cambios por la nueva función de LEDs.

## Requisito de cargador compatible

El flujo OTA con conservación del vínculo requiere un cargador que entregue el
aviso Service Changed correcto y comparta el formato de intercambio reconocido
por esta aplicación. El cargador anterior publicado, `v2.3-muzi.1`, usa el inicio
fijo 0x000C y oculta el error de atributo inválido; S140 6.1.1 reserva los
atributos hasta 0x000D. No se declara aprobado ese flujo con el cargador anterior.

La candidata `v2.3-muzi.2-rc1` utiliza el handle real asignado al servicio DFU.
Su UF2 tiene SHA-256
`5d8163cc5a4f28bef2639388c83a95c1b846ab4b8a2222efe963d8d0eda35114`.
Se instaló por USB con autorización del usuario; `INFO_UF2.TXT` confirmó esa
identificación y S140 6.1.1. La compilación y la auditoría estática pasan. El
instalable contiene el cargador con su configuración Muzi y no incluye la
aplicación ni SoftDevice. No se trata de una imagen debug.

**Un ciclo OTA con este cargador y la aplicación actual `1f0e400…` pasó.**
La firma de su rutina de intercambio coincide con la que reconoce MeshCore y se
comprobaron físicamente transferencia, validación, arranque automático y
reconexión con el vínculo conservado. El cargador compatible requerido para este
resultado es `v2.3-muzi.2-rc1`; no se extiende la aprobación al cargador anterior
ni a otras compilaciones de aplicación. El nombre de distribución previsto es
`Muzi-Works-SuperBase-Bootloader-v2.3-muzi.2-rc1.uf2`, con el contenido ensayado.
La futura distribución debe identificar esa dependencia y sus resultados de
validación. **La publicación continúa detenida por las demás validaciones
pendientes; estos archivos siguen sin publicar.**

## Validaciones antes de publicar

1. Instalar y verificar la corrección del GPS exclusivamente físico: Mode 1
   apaga, Mode 2 adquiere fix, centro no cambia el estado GPS y los dos modos se
   respetan al arrancar a batería. Completar sesión a batería. El fix y la
   alternancia física de `1f0e400…` están confirmados; no sustituyen esta prueba
   del nuevo binario. **La publicación continúa detenida.**
2. Distinguir el porcentaje mostrado por Android de la comprobación GATT BAS,
   cuya lectura/notificación física final sigue pendiente. Completar el recorrido
   físico de los modos selectivos del buzzer. La confirmación visual del joystick
   del nombre ya está aprobada en la imagen actual.
3. Mantener como aprobados en `1f0e400…` OTA con vínculo conservado, radio en
   ambos sentidos, seis controles, Atrás/centro, ausencia del aviso falso de carga,
   LEDs On/Off persistentes, Bluetooth Off/RESET/On y buzzer Off/RESET. No volver
   a presentar esas pruebas como pendientes ni extenderlas a otras compilaciones.
4. Revisar las mediciones y ensayos cuantitativos pendientes: consumo, descarga
   completa, alcance, precisión/retención RTC, fallos eléctricos y corte de
   alimentación al escribir. No presentar autonomía, alcance o robustez eléctrica
   como aprobados sin evidencia; las pruebas funcionales no aportan esas medidas.
5. Mantener identificados hashes, licencias, fuente y exclusión de diagnósticos.
   Actualizar el estado cuando se reanuden las pruebas; la preparación local de
   archivos revisables no constituye publicación ni aprobación como estable.

No forman parte del alcance: ACK por MQTT, cambios de seguridad propios de
Meshtastic, teclado autónomo, brújula ni temporizador estacionario de 12 horas.
No se han exportado identidad, claves ni configuración del nodo. Se conservan el
protocolo MeshCore y los parámetros de radio del usuario. Los arreglos de
persistencia CCCD no borran ni recrean los campos de claves o nombre del vínculo.
