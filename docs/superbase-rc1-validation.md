# Muzi Works SuperBase: revisión de alcance de RC1

Revisión de alcance iniciada: 27 de octubre de 2026, America/La_Paz.
Estado al cierre: 28 de octubre de 2026, America/La_Paz.
Versión: **v1.18-sb2-rc1**. Hardware: **Base Uno nRF52840/SX1262 + Super IO**.
Uso: Companion MeshCore por Bluetooth.

## Estado de la candidata

**Publicación detenida. No hay una candidata aprobada como estable.**
El alcance incluye las mejoras acordadas y las solicitudes expresas durante la
validación: Atrás para apagar la pantalla, LEDs persistentes, GPS exclusivamente
físico y páginas LEDs/Buzzer con iconos del estilo de Bluetooth, Hibernate y Advert.
El perfil normal usa optimización de
producción, sin comandos USB de diagnóstico ni trazas.
Los binarios internos, datos del nodo y registros de desarrollo no se publican.

**Corrección estética instalada por USB; estética rechazada por el usuario.** Las
páginas LEDs y Buzzer usan iconos monocromos de 32 × 32: bombilla redonda de
contorno grueso y base corta, altavoz sólido con las dos ondas separadas del
bitmap Advert, y variantes Off con diagonal y separación de fondo. Cada página
usa una sola leyenda en **y=53**, exactamente en la línea de Bluetooth,
Hibernate y Advert. No cambia controles, políticas de LEDs/sonido, GPS ni radio.

El ZIP de esta revisión tiene SHA-256
`c8bbb5fa95b7171ec11d4187572f3b8f9b9186e967fea81e3c7faed0bf13d487`;
su UF2 tiene SHA-256
`647a68be2a78e3a07cdb550fa6c1f00841b4edf45c5d49c0396907989e1dc937`.
Código compilado: `8ab756e9def03306a8a1330eea464f9c92e93736`.
Compilación ARM y auditoría UF2/DFU pasan, con 452.152 bytes de aplicación.
La transferencia USB y el retorno automático al firmware pasaron, sin RESET ni
reconectar el cable. El usuario indicó que todavía se ve extraño y pidió usar
exclusivamente el icono de Bluetooth como guía. **La estética de `c8bbb5…` no
está aprobada.** Se preparan comparaciones antes de volver a cargar firmware.

En la imagen anterior, ZIP
`711c33dc84666674d963efb480fd0ca75f1698762b2d44de69655951fec40c57`,
pasaron instalación USB, retorno automático y conexión MeshCore. El usuario
confirmó que los iconos se veían y sus controles funcionaban, pero **rechazó su
estética por no conservar el estilo de las páginas de referencia**. Ese resultado
es funcionalidad confirmada con estética rechazada, nunca un PASS visual.

Las preferencias finales solicitadas son **LEDs Off y Buzzer All**; su estado
tras la revisión `c8bbb5…` todavía no está confirmado. Los cambios desde el código
`949296…` son exclusivamente de presentación. Se conserva la evidencia funcional
identificada de GPS y radio de `13317e…`; no se exigen nuevas pruebas de esas
funciones por esta corrección visual ni se presentan como mediciones nuevas.

**Corrección GPS introducida en `13317e…`, solicitada el 28 de octubre:** el GPS debe obedecer
exclusivamente al interruptor físico (Mode 1 Off, Mode 2 On), también al arrancar.
La página pasa a ser informativa y las escrituras de activación GPS por Companion
se rechazan sin cambiar estado ni preferencias. La antigua preferencia guardada
deja de impedir el arranque GPS en Mode 2. El receptor integrado ya no recibe un
pulso de detección al arrancar en Mode 1. Los ensayos del ZIP `1f0e400…` siguen
identificados como evidencia anterior, no como aprobación automática del nuevo
binario.

El ZIP anterior con la corrección GPS tiene SHA-256
`13317e834cd4835b58febddcc82d2fca8ac0700d77c635b52af3423615c8230f`;
su UF2 corresponde a
`c0b3ad9c763b6d055a1359a3d5b1771f7c7358aa1b87200403fe331456387212`.
Código compilado: `949296ce97a79d5fa0b99deafe17ce8e9f869437`.
Compilación ARM y auditoría UF2/DFU pasan, con 451.976 bytes de aplicación.
Las regresiones Windows y ASan/UBSan Linux pasan: se compilan los métodos
reales de inicialización y control del GPS contra dependencias simuladas, en
SuperBase y en la variante heredada. Se comprueban arranque en ambos modos,
ausencia de sondeo de encendido en Mode 1, sondeo de switch cada 200 ms y
rollover, ausencia de arranques/paradas duplicados y rechazo de escrituras
GPS sin efectos. Los demás ajustes delegados conservan su comportamiento.

La transferencia USB y el retorno automático al firmware de este ZIP pasaron.
La app MeshCore llegó a su pantalla normal conectada, con 90% de batería mostrado,
sin reemparejamiento manual. Ese porcentaje no constituye una nueva comprobación
de notificaciones GATT BAS ni demuestra alimentación exclusivamente a batería.

**Confirmaciones del usuario en `13317e…`:** en Mode 2 la página muestra GPS On y
centro no lo cambia; en Mode 1 muestra GPS Off/no fix y centro tampoco lo cambia.
El usuario regresó a Mode 2. Se conservaron LEDs Off, el joystick del nombre se
dibujó correctamente y Atrás apaga la pantalla/centro la despierta.

**Arranque y GPS exterior básicos aprobados en `13317e…`:** el usuario confirmó
alimentación exclusivamente a batería, sin USB. Tras RESET en Mode 1 arrancó con
GPS Off/no fix y tras RESET en Mode 2 arrancó con GPS On. En exterior, a batería
y en Mode 2, recuperó fix con ocho satélites. Android volvió a la pantalla normal
conectada a SuperBase, con 74% mostrado después de esos arranques y sin nuevo
emparejamiento manual.

**Radio bidireccional aprobada en `13317e…`:** el mensaje
`RC1-GPS-SB-V4-01` de SuperBase a Heltec V4 quedó Delivered y se observó en V4;
`RC1-GPS-V4-SB-01` de V4 a SuperBase también quedó Delivered y se observó en
SuperBase, sin RESET antes de leer. **Todavía no se han probado en esta imagen
OTA ni GATT BAS. La sesión prolongada a batería sigue pendiente.** Este resultado
permanece atribuido a `13317e…`; la revisión visual no cambia el código GPS/RF.

Los resultados físicos siguientes no se transfieren automáticamente a cada
recompilación. Se identifica qué implementación se ensayó y qué falta repetir
sobre el instalable final. Compilación y auditoría no certifican el hardware.

Las pruebas físicas históricas siguientes corresponden al ZIP de producción anterior con SHA-256
`1f0e400df7e9b6052608aed4380e55fe3183d0b6aba55f85cf15a4445b87d943`,
compilado desde la revisión de código `4a7577d37612becba55ebc431ea71313ada2b999`.
El UF2 correspondiente tiene SHA-256
`54739510b7c3b1cfa088b03d6312fe20f9b25fc7e9e8119750ea2146d0754028`.
Incluye Atrás para apagar pantalla y la nueva página de LEDs. Compilación ARM y
auditoría del paquete pasan. En este archivo exacto quedaron comprobados los
seis controles, Atrás para apagar pantalla y centro para despertarla, ausencia
del aviso falso del cargador, LEDs On/Off persistentes tras RESET, Bluetooth Off
y buzzer Off persistentes tras RESET y posterior activación de Bluetooth.

**La OTA del archivo anterior `1f0e400…` pasó:** con Nordic predeterminado y cargador
`v2.3-muzi.2-rc1`, USB regresó automáticamente en **49,24 s**. MeshCore reconectó
con el mismo vínculo, cuyo último evento BONDED seguía en **00:07:18.398**, sin
nuevo PIN, RESET, reconectar USB ni cambiar ajustes del teléfono. LEDs Off se
conservó durante esa OTA. El cargador ensayado tiene SHA-256
`5d8163cc5a4f28bef2639388c83a95c1b846ab4b8a2222efe963d8d0eda35114`.

En `1f0e400…` también pasó radio bidireccional con Heltec V4: SuperBase → V4 y la respuesta
V4 → SuperBase quedaron Delivered y se observaron en cada receptor. Con buzzer
Off y LEDs Off, el usuario confirmó recepción silenciosa y sin destello verde.
Con buzzer All, una respuesta recibida quedó Delivered y el usuario oyó el aviso;
el verde permaneció apagado. Los cinco nombres de modo de buzzer se recorrieron
en pantalla; esto no equivale a ensayar físicamente cada filtro de los tres
modos selectivos.

Como antecedente del GPS, sin sustituir la prueba actual de `13317e…`:
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
anterior es evidencia histórica; la aprobación OTA de `1f0e400…` corresponde
al ensayo de 49,24 s descrito arriba. Ninguno aprueba OTA en `13317e…`.
Ambos se hicieron con USB conectado y no
acreditan autonomía ni las demás funciones pendientes.

GitHub Actions no ejecutó el flujo nuevo porque aún no existe en la rama
predeterminada. Las compilaciones ARM, regresiones Windows y sanitizadores
Linux/WSL se realizaron localmente. No se atribuye aprobación de CI.
Las correcciones de suscripciones Bluetooth descritas abajo pasaron las
regresiones nativas, AddressSanitizer/UndefinedBehaviorSanitizer y la compilación
ARM. La comparación del vínculo antes y después del ciclo OTA de `1f0e400…` confirmó
el mismo evento de emparejamiento y conexión MeshCore sin una nueva solicitud.

## Implementación y verificación

| Mejora acordada | Implementación y evidencia | Validación pendiente |
| --- | --- | --- |
| Iconos de LEDs y Buzzer, solicitud visual | `711c33…`: iconos y controles funcionaban, pero el usuario rechazó la estética; no es PASS visual. `c8bbb5…` instalada: bombilla gruesa/base corta, altavoz sólido y ondas exactas de Advert separadas; variantes Off, una sola leyenda y=53 como las referencias. ARM y auditoría pasan; instalación USB y retorno automático sin RESET/reconexión pasan. | Estética de `c8bbb5…` rechazada; revisar con Bluetooth como única referencia. LEDs Off y Buzzer All son los valores finales solicitados, todavía sin confirmar tras esta revisión. |
| Batería Bluetooth | BAS 0–100, reintentos y nueva suscripción; regresiones de entrega. La lectura/notificación GATT BAS en Windows corresponde a imágenes internas anteriores. En `13317e…`, Android mostró 90% tras USB y 74% después de los arranques a batería; esos datos no demuestran por sí solos lectura/notificación BAS. | Repetir la comprobación GATT BAS sobre el instalable final `c8bbb5…`. Calibración si se promete autonomía. |
| Cargador BQ25185 | STAT1 con pull-up y distinción carga/reposo/fallo. En `1f0e400…`, el usuario confirmó desaparición de `! Check charger`; entradas y fallos simulados cubiertos. | La revisión visual `c8bbb5…` no cambia el control del cargador ni añade una nueva medición física. Fallos eléctricos inducidos y corriente de carga sin medir. |
| Bluetooth desde pantalla | Habilitación en ejecución y preferencia persistente. En `1f0e400…`, Bluetooth Off persistió tras RESET; el usuario lo volvió a On y MeshCore conectó. La reconexión después de OTA conservó el vínculo. En `13317e…`, MeshCore conectó normalmente después de USB y de los arranques a batería, sin reemparejamiento manual. | Las conexiones actuales no repiten Off/RESET/On ni conservación del vínculo durante OTA en `13317e…`; no se extrapola a todos los teléfonos. |
| Buzzer | Modos All, Off, Messages, System y DMs Only; regresiones de políticas y propiedad. En `1f0e400…`, el usuario vio los cinco nombres y Off persistió tras RESET. Un DM confirmado Delivered y visto en SuperBase fue silencioso en Off; otro DM confirmado Delivered produjo aviso audible en All. | Ensayo físico de los filtros Messages, System y DMs Only; sus nombres en pantalla no prueban su selección de avisos. |
| Seis controles y Atrás para apagar pantalla | Captura de flancos, rebotes, pulsación larga, triple Atrás y sondeo alternativo; regresiones. En `1f0e400…`, el usuario confirmó los seis controles. En `13317e…` volvió a confirmar Atrás para apagar y centro para despertar; centro en la página GPS no cambia su estado en ninguno de los dos modos. El CLICK corto de Atrás se consume y no despierta una pantalla ya apagada. Triple Atrás y pulsación larga se conservan. Captura interna previa: máscara 63, dos flancos adicionales por control, cero descartes. | Atrás/centro y el carácter informativo de la página GPS están aprobados en `13317e…`; no se presenta la prueba histórica de los seis controles como nueva captura física ni se afirma captura infalible durante operaciones bloqueantes. |
| LEDs desde la pantalla, solicitud nueva | Página LEDs junto a sonido, centro alterna On/Off y guarda `comp.led_en`, predeterminado On. Verde P1.03/GPIO 35; azul P1.04/GPIO 36 se mantiene apagado. En `13317e…`, el usuario confirmó que LEDs Off se conservó tras instalar por USB. En `1f0e400…`, On restauró el parpadeo y persistió tras RESET; Off persistió tras RESET/OTA y no hubo destello verde durante DMs. | Off confirmado en `13317e…`; todavía no se repiten On/Off tras RESET ni Off durante OTA en `13317e…`. Rojo de carga y destellos del cargador de arranque quedan fuera; véanse los límites de hardware. |
| GPS | `13317e…` instalada y auditada: control exclusivamente físico, rechazo de escrituras GPS y página informativa. El usuario confirmó Mode 2 On y Mode 1 Off/no fix; centro no cambia ninguno. A batería y sin USB, RESET en Mode 1 arrancó Off/no fix y RESET en Mode 2 arrancó On. En exterior a batería recuperó fix con ocho satélites. Los métodos reales de arranque/control pasan pruebas nativas y sanitizadores. | Arranque por switch y adquisición exterior básica aprobados en `13317e…`. Sesión prolongada a batería pendiente; no se promete recepción interior. |
| Energía y apagado | Reposo IMU/magnetómetro y parada de buzzer/pantalla/GPS observados en imágenes internas. En `13317e…`, el usuario confirmó alimentación exclusivamente a batería, reinicios en ambos modos y nuevo fix exterior; MeshCore reconectó sin nuevo emparejamiento manual. | Sesión prolongada a batería, descarga completa y consumo instrumental sin medir. Los reinicios básicos están confirmados en `13317e…`. |
| Preferencias | Temporal + rename, reintento y vaciado previo; pruebas de apertura, escritura incompleta, borrado y rename fallidos. En `1f0e400…`, LEDs On/Off, Bluetooth Off y buzzer Off persistieron tras RESET; LEDs Off también durante OTA. La preferencia GPS heredada se conserva por compatibilidad, pero se ignora como puerta: en `13317e…` se confirmó el arranque según el switch en ambos modos. | La persistencia histórica de LEDs/Bluetooth/buzzer no se transfiere automáticamente a `13317e…`. Corte de alimentación durante escritura sin ensayar. Contactos heredados no informan todos sus errores; no hay transacción conjunta. |
| Radio SX1262 | Recuperación RX y protección TX con fallos simulados. En `13317e…`, `RC1-GPS-SB-V4-01` SuperBase → V4 y `RC1-GPS-V4-SB-01` V4 → SuperBase quedaron Delivered y se observaron en cada receptor, sin RESET antes de leer. En `1f0e400…` se había probado además recepción con buzzer Off/All y LEDs Off. No se modificaron parámetros de radio. | Mensajería bidireccional de `13317e…` aprobada y código RF conservado en `c8bbb5…`; el cambio exclusivamente visual no requiere repetirla. Alcance máximo y fallo físico inducido sin ensayar. |
| Actualización Bluetooth | Guardado previo, corrección de USB y RAM de intercambio, autorización S140 y entrada cifrada con vínculo compartido. Ambos arreglos CCCD pasan pruebas nativas, sanitizadores y ARM. **Ciclo anterior aprobado:** ZIP `1f0e400…`, cargador `v2.3-muzi.2-rc1` SHA `5d8163…`, Nordic predeterminado, USB automático en 49,24 s y mismo último BONDED `00:07:18.398`, sin nuevo PIN, RESET, reconectar USB ni cambiar ajustes del teléfono. LEDs Off sobrevivió. En `13317e…` solo se ha completado instalación USB, retorno automático y conexión normal MeshCore. | Repetir ciclo OTA y reconexión con vínculo conservado en el instalable final `c8bbb5…`; no se registra un nuevo ciclo en las revisiones visuales. La aprobación de `1f0e400…` no se transfiere a esta imagen ni acredita autonomía u otros teléfonos. Una nueva petición de emparejamiento sería un fallo, no un paso normal. |
| Emojis | UTF-8, 3.953 secuencias OpenMoji 17, recorte y saltos sin partir secuencias. Catálogo y límites con sanitizadores. El usuario confirmó de nuevo el joystick del nombre correctamente dibujado en `13317e…`. | Confirmación visual del joystick aprobada en `13317e…`; no se ensayaron físicamente los 3.953 pictogramas. Monocromo de 10 × 10: no conserva color ni todo el detalle. |
| RTC | Se conserva el reloj heredado. Lectura y avance de segundos RX8130CE observados en imagen interna. | Precisión y retención no medidas; no se añade otra integración de hora. |

## Fallos y efecto real

- **Aviso falso de cargador:** corrección observada y cubierta por regresiones.
- **Bloques del emoji:** corrección observada; se conserva el nombre UTF-8.
- **Radio sin confirmación inicial:** resuelto tras intercambiar anuncios; no
  se reprodujo como fallo persistente. `13317e…` repitió comunicación
  bidireccional con Heltec V4, Delivered y observada en ambos receptores.
- **Captura por interrupciones:** los contadores a cero de una prueba anterior
  no describen la última evidencia. Los seis controles produjeron flancos reales
  antes de instalar RC1. Se mantiene el sondeo alternativo.
- **Pantalla negra:** el apagado por inactividad se distingue del bloqueo DFU.
  En `13317e…` pasó el retorno automático tras USB y el usuario confirmó despertar
  con centro. El retorno sin RESET/reconectar USB durante OTA corresponde a `1f0e400…`.
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
  vínculo durante aquel fallo. **El ciclo OTA anterior de `1f0e400…` conservó
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
  se modifica. **La reconexión tras OTA del ZIP anterior `1f0e400…` pasó con el
  mismo vínculo y sin otro PIN.** La evidencia de `ec7f774…` se conserva como
  antecedente de ese ciclo de `1f0e400…`; ninguno aprueba OTA en `13317e…`.

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

**Un ciclo OTA con este cargador y la aplicación anterior `1f0e400…` pasó.**
La firma de su rutina de intercambio coincide con la que reconoce MeshCore y se
comprobaron físicamente transferencia, validación, arranque automático y
reconexión con el vínculo conservado. El cargador compatible requerido para este
resultado es `v2.3-muzi.2-rc1`; no se extiende la aprobación al cargador anterior
ni a otras compilaciones de aplicación. El nombre de distribución previsto es
`Muzi-Works-SuperBase-Bootloader-v2.3-muzi.2-rc1.uf2`, con el contenido ensayado.
La repetición OTA con el instalable final `c8bbb5…` permanece pendiente;
tampoco se realizó en `13317e…`.
La futura distribución debe identificar esa dependencia y sus resultados de
validación. **La publicación continúa detenida por las demás validaciones
pendientes; estos archivos siguen sin publicar.**

## Validaciones antes de publicar

1. `c8bbb5…` ya está instalada, con transferencia USB y retorno automático sin
   RESET/reconectar. Falta aprobación estética del usuario y confirmar los valores
   solicitados LEDs Off/Buzzer All. `711c33…` funcionaba, pero su estética fue
   rechazada; no se registra como PASS visual. ARM y auditoría no sustituyen
   esa aprobación.
2. En la imagen anterior `13317e…` se confirmaron Mode 1 Off/no fix,
   Mode 2 On y centro sin cambiar GPS; también reinicios exclusivamente a batería
   en ambos modos y nuevo fix exterior con ocho satélites. No volver a presentar
   esas pruebas básicas como pendientes. Queda la sesión prolongada a batería.
   **La publicación continúa detenida.**
3. Distinguir el porcentaje mostrado por Android de la comprobación GATT BAS,
   cuya lectura/notificación física final sigue pendiente. Completar el recorrido
   físico de los modos selectivos del buzzer. La confirmación visual del joystick
   del nombre ya está aprobada en `13317e…`.
4. Completar OTA con vínculo conservado sobre el instalable final; no se ha
   realizado de nuevo en `c8bbb5…`. Radio bidireccional y GPS de `13317e…` ya
   están aprobados y su código se conserva; no se abren nuevas pruebas funcionales
   por cambiar solo presentación. Se identifica la imagen realmente ensayada,
   sin convertir evidencia histórica en mediciones nuevas. Mantener como
   aprobados en `1f0e400…` OTA y radio en
   ambos sentidos, seis controles, Atrás/centro, ausencia del aviso falso de carga,
   LEDs On/Off persistentes, Bluetooth Off/RESET/On y buzzer Off/RESET. No volver
   a presentar esas pruebas históricas como pendientes ni extenderlas a otras
   compilaciones. En `13317e…` están confirmados joystick, Atrás/centro y LEDs Off
   conservado tras USB; la persistencia de On tras RESET y Off durante OTA no se
   han repetido todavía.
5. Revisar las mediciones y ensayos cuantitativos pendientes: consumo, descarga
   completa, alcance, precisión/retención RTC, fallos eléctricos y corte de
   alimentación al escribir. No presentar autonomía, alcance o robustez eléctrica
   como aprobados sin evidencia; las pruebas funcionales no aportan esas medidas.
6. Mantener identificados hashes, licencias, fuente y exclusión de diagnósticos.
   Actualizar el estado cuando se reanuden las pruebas; la preparación local de
   archivos revisables no constituye publicación ni aprobación como estable.

No forman parte del alcance: ACK por MQTT, cambios de seguridad propios de
Meshtastic, teclado autónomo, brújula ni temporizador estacionario de 12 horas.
No se han exportado identidad, claves ni configuración del nodo. Se conservan el
protocolo MeshCore y los parámetros de radio del usuario. Los arreglos de
persistencia CCCD no borran ni recrean los campos de claves o nombre del vínculo.
