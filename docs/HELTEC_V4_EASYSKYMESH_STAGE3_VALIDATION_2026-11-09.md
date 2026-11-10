# Heltec V4: cierre de validación de la tercera etapa EasySkyMesh

**Validación funcional acotada completada.** El Heltec terminó con el normal `v1.1.2-tf04db3d3`, aplicación exacta `0eb0e90e…`, RF final 16/16 y confirmación del usuario de teléfono/OLED/PRG. La fuente normal f04 aprobó 196 pruebas nativas y diez roles/veinte imágenes; debug con SDK normal recibió 64/64 durante intervalos con sueño MCU. PM LAB P0 completó 16/16; P1 conserva su FAIL inicial 7/8 y repetición idéntica 16/16; P2 completó 32/32, residencia nueva APB_MIN configurada a 40 MHz, guardas, GATT y restauraciones.

Este cierre se limita a las condiciones documentadas. El SDK personalizado permanece **sólo en LAB**, fuera del normal y de sus valores predeterminados; no se declara estable ni se midió corriente. Tampoco se obtuvo fix GPS/UTC físico. `main`/estable v1.1.2 permanecieron en `62e184a6f104a0f75bb5054e64471275c553b6e8`: no se hizo merge o release nuevo. La fuente de firmware validada es `f04db3d3b70785642a0ee754e62366b0da96b1bf`; una posterior modificación exclusivamente documental no recompila ni cambia sus imágenes.

El [cierre conjunto](../out/easyskymesh-stage3-2026-11-09/joint-f04-validation-closure-1794277046637212400.json), SHA256 `4ecc7a2a4e80922cd6dc4f3b8a63e4ae7a61dec3db43931198af5e00cb16f8e9`, confirma los componentes y sus hashes sin alterar los intentos históricos. El [resumen JSON redactado](HELTEC_V4_EASYSKYMESH_STAGE3_VALIDATION_2026-11-09.json) acompaña esta narrativa. Los registros completos se mantienen en OUT, ignorado por Git; se citan rutas/hashes locales, sin publicar identificadores, PIN, claves o coordenadas. El [índice final](../out/easyskymesh-stage3-2026-11-09/stage3-final-evidence-index-r3.json) conserva los cortes anteriores intactos.

Antecedentes del alcance: [comparación original EasySkyMesh](HELTEC_V4_EASYSKYMESH_COMPARISON_2026-11-09.md) y [integración/validación de etapas1–2](HELTEC_V4_EASYSKYMESH_INTEGRATION_2026-11-09.md). Se distingue la evidencia nueva de los resultados previos; ninguna compilación sustituye una prueba física pendiente.

## Fuente, imágenes y conservación

| Identificador | Estado y bytes concretos |
|---|---|
| Rama de integración | `integrate/easyskymesh-selective` |
| Fuente normal | `f04db3d3b70785642a0ee754e62366b0da96b1bf` |
| Árbol | `fc330fe6fb0e6fe46c1873781ab5e76048a41a6b` |
| Normal de validación | `v1.1.2-tf04db3d3`; BLE OTA1.634.816 bytes, SHA256 `0eb0e90e2b0bb3f383fe6ae72421c4d63787e98a6cc9dfbfd79cbc73c66dfa8c` |
| Debug de batería | `v1.1.2-df04db3d3`; BLE OTA1.636.608 bytes, SHA256 `55b174f068be0c7ef278cde55d36261092f853e0f0981a54ae8c1f4f75569d7c`; hooks MCRXTEST presentes sólo aquí |
| PM LAB P0 | `v1.1.2-pf04db3d3`;80/80 MHz, modem sleep OFF. OTA1.648.208 bytes, SHA256 `9a40329c1c1b30ca694ae7c6896d50cc99abbb93388dc116651ced70227447ef`; ensayo funcional completo |
| PM LAB P1 | Misma etiqueta PM,80/80 MHz, modem sleep ON. OTA1.648.272 bytes, SHA256 `c3d4d453ccf4073910fb84d692581460b7a2c16a4efe68346203d914f5a7125c`; primer FAIL conservado y repetición completa |
| PM LAB P2 | Misma etiqueta PM,80/40 MHz, modem sleep ON. OTA1.648.272 bytes, SHA256 `6d98e603073c2c35320a38d75c97eb2ed255455553417fc126734f037692562f`; ensayo funcional completo, retirado del DUT al restaurar normal |
| Manifiesto normal | [`candidate-build-results.json`](../out/easyskymesh-stage3-2026-11-09/candidate-build-results.json), SHA256 `05b78226eade7871e25835c5a3a310a62f465f9c1267000062e8c1de4775de65` |

Las veinte imágenes son OTA y merged para cada uno de los diez roles existentes, no veinte variantes. Roles: Solar Repeater, Expansion Kit Repeater, Repeater Bridge ESP-NOW, Room Server, Terminal Chat, Companion USB, Companion BLE, Companion Wi-Fi, Sensor y KISS Modem. Los tres binarios PM no son roles normales nuevos. El manifiesto confirma source limpio, matriz completa, layout común, aplicación que cabe en los slots, versiones y ausencia de hooks de prueba en imágenes normales. Los assets estables permanecieron sin modificar. La [lista de hashes normal](../out/easyskymesh-stage3-2026-11-09/candidate-SHA256SUMS.txt) tiene SHA256 `f0fa940edf482ebf0733ab8439a8e161a9dc5187e8b9e6e54d833638c3b3fad8`.

La instalación normal y la transición a debug escribieron solamente la aplicación de app 0. Las herramientas verificaron fuente/imagen previa exacta, chip y eFuse autorizados, tabla, OTADATA/CRC/slot y MD5 en el equipo de bootloader, tabla, NVS, OTADATA, SPIFFS, coredump y app 1 antes/después. No crearon respaldos, no exportaron esos datos ni hicieron erase completo. Un reset enviado desde ROM no se equiparó a boot: el arranque se comprobó por separado. Ver [`candidate-installation.json`](../out/easyskymesh-stage3-2026-11-09/candidate-installation.json), el [intento de transición a debug](../out/easyskymesh-stage3-2026-11-09/controlled-app-transition-attempt-1794272666174571600.json) y el [probe USB debug](../out/easyskymesh-stage3-2026-11-09/debug-usb-probe-1794273091144490000.json).

El probe USB confirmó la versión debug y estado esperado, sin transmisión RF, persistencia de ajustes ni acceso a la referencia; la activación BLE solicitada fue volátil. El normal ya fue restaurado y comprobado por separado al terminar LAB; el usuario confirmó teléfono/OLED/PRG. El debug y PM LAB no son assets de distribución estable.

## Qué contiene la fuente normal f04

- Planificador GPS opcional con estados Off/Continuous/Acquiring/Resting. Mantiene separado el pedido del usuario del receptor realmente despierto. El opt-in permanece apagado por defecto; un perfil anterior con GPS habilitado continúa en modo continuo.
- Adquisición máxima predeterminada 120 s, descanso 900 s y hold de fix 5 s. Rangos estrictos: adquisición 5–3600 s, descanso 1–86400 s y hold 0–60 s, no mayor que adquisición. Cambios inválidos se rechazan antes de modificar runtime/preferencias. Se conserva el cero legacy del intervalo guardado y su equivalente runtime de un segundo.
- `gps off` no inicia UART ni sondea el módulo en arranque, permanece autorizado como apagado durante descanso y no se revierte por consultas. `gps sync` sólo puede interrumpir un descanso cuando el usuario mantiene GPS habilitado. Desactivar el ciclo o el ahorro global aplicable reconcilia inmediatamente un GPS autorizado que estaba durmiendo.
- Fix nuevo identificado por secuencia GGA/RMC, edad de fix de 64 bits, caché preservada antes de apagar y sin despertar por telemetría. No se impone la caducidad externa en 2030. Se valida calendario antes de actualizar RTC y se mantiene el epoch `uint32_t` de RTC incluso cuando el getter histórico `long` tiene signo y 32 bits después de 2038.
- Comandos GPS compartidos por CLI y app, esquema persistido `gps:{cycle:{en,acquire,rest,hold}}`, normalización segura y persistencia que conserva la posición estática guardada. Los diagnósticos no imprimen coordenadas.
- `get radio.cad`: contador saturado de ocupación CAD real y estado enabled, sin activar CAD ni modificar la radio. En los ensayos físicos de esta fase CAD siguió deshabilitado y busy 0: se comprobó lectura/continuidad, no contención CAD física.
- Estado de lectura ocupada en UART/Wi-Fi/BLE/Multi para no iniciar reposo entre fragmentos de una trama. Incluye expiración acotada de un fragmento abandonado y recuperación. Las guardas existentes de RX continuo, TX máximo, OLED/PRG, USB y BLE se conservan.

La cobertura final se ejecutó sobre la fuente congelada mediante los seis entornos de validación; [`native-tests-final.log`](../out/easyskymesh-stage3-2026-11-09/native-tests-final.log) registra **196/196 PASS en 56,726 s**. Incluye serializadores/NodePrefs reales, colas, PATH/ANON, planificador GPS, comandos y persistencia, gestores reales con fronteras simuladas, driver/contador CAD, parseo de interfaces y MicroNMEA real. Los doce casos MicroNMEA probaron parser, checksum/fixseq, edad/caché, rail compartido, sincronización/calendario y límites de relojes; el DateTime usado en ese host es una frontera fake y no una prueba de RTClib. La selección inicial de MicroNMEA en native falló por compatibilidad de framework y se corrigió sólo en la configuración del test; ese log se conserva.

## Condiciones y resultados físicos normales

El DUT fue el Heltec V4 de alta potencia; la referencia fue el MuziWorks SuperBase Companion `v1.18-sb2-rc1`, sin cambiar firmware o configuración. MeshMonitor es otro dispositivo y quedó excluido. Se omitieron del informe MAC, PIN, identidad y ubicaciones. Los scripts se limitan a los endpoints autorizados.

La RF se mantuvo en 910,525 MHz/BW62,5 kHz/SF7/CR5, perfil `range`, SX1262 boosted y FEM/LNA habilitados. El diagnóstico mostró solicitud 22 dBm, entrada aplicada 21 dBm y salida **estimada**28 dBm por la tabla FEM; no es potencia RF medida. Los intercambios RAW prueban radio y transporte de diagnóstico con contenido conocido, sin consultar mensajes/FIFO/contactos/canales o claves. Dos radios no constituyen una validación de retransmisión controlada con tres nodos ni de todas las funciones de la aplicación.

| Ensayo | Resultado conservado |
|---|---|
| RF con GPS continuo encendido, ventana 2,5 s | [`696a7a`](../out/easyskymesh-stage3-2026-11-09/rf-candidate-gps-on-attempt-1794271120127284000-696a7a.json): **16/16 estricto PASS**, ocho por dirección, sin duplicados/tardíos y delta RXerrors0 en ambos. UART GPS avanzó durante el ensayo; se restauró el pedido GPS original |
| RF normal posterior, ventana 2,5 s | [`08e977`](../out/easyskymesh-stage3-2026-11-09/rf-candidate-attempt-1794272163079418700-08e977.json): **FAIL**. Llegaron 16/16 eventualmente, pero sólo 15/16 dentro de ventana; referencia→DUT #8 llegó a4144,434 ms. Hubo además delta de un error RX en referencia |
| Repetición normal, ventana 5 s | [`708959`](../out/easyskymesh-stage3-2026-11-09/rf-candidate-attempt-1794272407458934800-708959.json): **16/16 PASS**, ocho por dirección, sin tardíos/duplicados y delta RXerrors0 en ambos |

La repetición amplió la ventana: **no demuestra 16/16 a2,5 s ni convierte 08e977 en aprobado**. No se identificó una causa para el retorno tardío o el error RX de referencia, ni una relación causal entre ambos. No se redujo TX/RX para obtener el resultado. Los errores RX históricos de la referencia no se borraron; se evaluaron deltas del ensayo. Los intentos y registros anteriores, incluido el primer control estable 13/14 de etapas anteriores, siguen conservados y no se suman como aprobaciones de esta fase.

### GPS físico

El primer [intento543e24](../out/easyskymesh-stage3-2026-11-09/gps-physical-attempt-1794271303150380500-543e24.json) falló en conexión/preflight, antes de pasos y sin modificar ajustes GPS. Queda como FAIL de transporte, sin adjudicar una causa al firmware. La ruta Windows para vínculo ya conocido se preparó aparte, manteniendo las comprobaciones de destino/identidad/versión y sin modificar el firmware; no se afirma con ello recuperación BLE infalible.

El [ensayo d55572](../out/easyskymesh-stage3-2026-11-09/gps-physical-attempt-1794271968512152100-d55572.json), también guardado como [`gps-physical-validation.json`](../out/easyskymesh-stage3-2026-11-09/gps-physical-validation.json), aprobó las once comprobaciones funcionales: apagado sin wake por consultas ni sync, NMEA físico/checksums, rechazo atómico de valores, entrada desde app, timeout a descanso, consultas sin wake, ciclo OFF→continuo, apagado del usuario durante descanso, sync autorizado, RF/runtime conservados y restauración/boot de preferencias originales.

En el tramo continuo se registraron 379 bytes UART y 11 sentencias con checksum válido; al observar el descanso,2223 bytes y 58 sentencias válidas. Esto confirma actividad del módulo/UART bajo los pines y alimentación usados. El módulo no aportó un fix calificado: sat 0, edad `never`, `fresh_fix_during_cycle_qualified:false`. No se validaron posición exterior, precisión, tiempo UTC obtenido físicamente ni finalización por fix fresco; el descanso observado corresponde a adquisición acotada. No se registraron coordenadas.

Se restauraron los ajustes originales GPS request 0/cycle 0/acquire 120/rest 900/hold 5 y la vista de ajustes de app, con verificación tras reboot. Este ensayo GPS no midió sueño MCU o corriente. El usuario no puede trasladar el nodo a cielo abierto en estas condiciones: posición/fix fresco y UTC físico siguen **sin calificar**. La posterior prueba de batería descrita abajo sí observa sueño MCU, pero no convierte el timeout GPS en una finalización por fix.

## Batería con debug f04:64 recepciones y sueño MCU observado

El [resultado canónico](../out/easyskymesh-stage3-2026-11-09/sleep-gps-debug-evidence.json), SHA256 `2451fd8ef8edffc8db8f0752321ed3c59d498c89ac8c2eeeda5450d1ff2e2dfc`, es PASS. Se usó el debug `v1.1.2-df04db3d3` con el SDK normal y collector de diagnóstico, **no el SDK personalizado PM**. USB estuvo ausente antes/después de las ventanas; BLE del DUT se apagó para permitir sueño manual. La referencia transmitió 32 RAW únicos por fase, con separación 2 s, sin leer mensajes/FIFO/contactos/claves. Los mapas recibidos fueron exactos 32/32 en ambas fases, duplicados 0, delta RXerrors0 en ambos y 32 TX directos efectivos de referencia por fase.

| Estado GPS durante batería | Recepción | Sueño observado en el intervalo | Ventana observada |
|---|---|---|---|
| Usuario GPS OFF,cycle 0 |32/32 exactos |4417 sleeps;28 GPIO+4389 timer;88.527 ms acumulados |128,279 s |
| Usuario GPS ON,cycle 1,estado resting/receptor awake 0 |32/32 exactos |3957 sleeps;30 GPIO+3927 timer;79.301 ms acumulados |107,879 s |

Se conservaron RF/perfil/TX máximo estimado, errores core/BLE/sueño 0 y uptime continuo. Los ajustes temporales GPS se restauraron a request 0/cycle 0/acquire 120/rest 900/hold 5; la vista app conservó intervalo 1, pantalla habilitada y permiso runtime de sueño 1. `settings_restored/gps_restored/boot_verified=true` acredita restauración y comprobación tras reboot del debug. El binario normal seleccionado volvió y aprobó arranque/RF final; la posterior confirmación manual también aprobó teléfono/OLED/PRG.

Los contadores prueban sueño y recepción conocidos durante **los intervalos observados**; no sitúan cada paquete en un instante exacto de sueño ni atribuyen cada wake GPIO a LoRa. No se midieron corriente, duración de batería ni pérdida de sensibilidad. Tampoco son una prueba física de cada frame serie fragmentado o de `isReadBusy` en UART/Wi-Fi/BLE/Multi: esa cobertura de bordes sigue siendo host. La imagen normal final tiene otros bytes; no se presenta el collector debug como una función distribuida normalmente.

## Framework PM: separado del candidato normal

Se construyó un SDK propio con fuentes fijadas de IDF4.4.7/`38eeba213…`, Arduino 2.0.17/`dcc1105b…`, lib-builder y componentes, bajo herramientas/venv privados WSL. Se preservó el SPI con backport 9333 (SHA c579…) y se deshabilitó DFS automático antes del setup. Se verificaron PM, profiler y BLE modem-sleep en el perfil S3 qio_qspi y bootloader QIO80. El framework normal quedó intacto. No se reutilizaron binarios del ZIP PS18: su inspección encontró diferencias que impiden usarlo como sustituto confiable.

Paquete privado: [`framework-package-manifest.json`](../out/easyskymesh-stage3-2026-11-09/framework-lab/results/framework-package-manifest.json), SHA256 `05f34b0a2d01b8a5ad8d335c3774fb0b534b4e1fb6e38d685a0307c60f892c1b`;3421 archivos,207.721.245 bytes. Una receta oficial selectiva repetida, con objetos borrados y ccache desactivado pero mismas rutas, dio 2349/2350 archivos idénticos. Todas las bibliotecas/headers coincidieron; `qio_qspi/sections.ld` intercambió dos cláusulas rodata y puede alterar direcciones al enlazar. Se conservó el [diff](../out/easyskymesh-stage3-2026-11-09/framework-lab/results/sections-rebuild.diff) y la [comparación](../out/easyskymesh-stage3-2026-11-09/framework-lab/results/sdk-clean-rebuild-comparison.json). **No hay reproducibilidad completa byte a byte ni entre hosts/directorios.** El paquete exportado original permanece inmutable.

El prototipo de aplicación está sólo en una copia LAB: HAL Arduino RadioLib protegido por APB antes/después de cada transacción real, bootguard hasta SPI listo y guards USB/GPS/OLED/ADC, fallback 80/80, control de errores, hooks BLE sin cambiar GATT, `get power.pm` y `get power.pmstats`. El profiler privado se fijó al SDK/ABI4.4.7 y captura sólo cuatro filas de modo en 512 bytes, sin nombres/direcciones de locks. `APB_MIN` debe compararse por delta posterior a configuración estable y frecuencia de fila 40; no basta el acumulado o el mínimo muestreado desde una tarea. `light_us:na` no representa sueño manual 20 ms ni corriente.

El parche LAB está fijado a `e2d9f37c49cc80c791c6e1a022eae92163788f0ac8ab421e54f39350259b0974`. Sus 18 casos de parser y 48 escenarios PM/HAL aprobaron **66/66 host**, con fronteras SDK/Arduino simuladas, separados de los 196 tests normales. Ver [manifiesto nativo LAB](../out/easyskymesh-stage3-2026-11-09/framework-lab/firmware-instrumentation/native-tests/results/1794272376902554800/verification-manifest.json). Los programas de ensayo tienen además sus propias pruebas puras de preparación; no se suman como cobertura física ni de firmware normal.

Las tres imágenes LAB tienen la misma versión visible PM. Su SHA/tamaño/manifiesto y el getter p0/p1/p2 distinguen el experimento; BLE no lee el digest de flash. Las instalaciones app-only verifican esos bytes por separado, preservan regiones no-app/app 1 y no afirman boot a partir de un reset ROM. Las pruebas posteriores de metadata/runtime comprueban el arranque real. No se exportan NVS/SPIFFS ni se crean nuevos respaldos.

| Ensayo LAB | Evidencia real y límite |
|---|---|
| P0:80/80,modem sleep OFF | [befff929…](../out/easyskymesh-stage3-2026-11-09/pm-stage0-attempt-1794274002436457200-ee0d2c/pm-stage0-validation.json): **COMPLETE**,16/16 RAW con ventana 5 s, delta RXerrors0 en ambos, USB/GPS/OLED guards correctos, BOOT liberado, SPI positivo/equilibrado y errores core/BLE/PM 0. BLE-off→PRG→nueva conexión emparejada conservó handles GATT/metadata; GPS/display/runtime restaurados. No hubo sueño MCU con BLE conectado/USB |
| P1 primer intento:80/80,modem sleep ON | [7a3cbd30…](../out/easyskymesh-stage3-2026-11-09/pm-stage1-attempt-1794274858008059200-6ae800/pm-stage1-validation.json): **FAIL** en DUT→referencia #8. DUT transmitió 8 y referencia recibió 7; sin recepción tardía en drenado ni errores RX nuevos. Se detuvo antes de la otra dirección/ciclo GATT. PM seguía saludable; GPS/display/runtime restaurados y cierre sin errores |
| P1 repetición idéntica | [65d96136…](../out/easyskymesh-stage3-2026-11-09/pm-stage1-attempt-1794275019721691200-f80b98/pm-stage1-validation.json): **COMPLETE**,16/16 RAW, ocho por dirección, misma imagen/harnés/RF/configuración/ventana 5 s; sin nuevos errores RX ni duplicados, errores core/BLE/PM 0, GATT estable, guardas y restauración verificadas |
| P2:80/40,modem sleep ON | [6f0af670…](../out/easyskymesh-stage3-2026-11-09/pm-stage2-attempt-1794276108810686300-a26c12/pm-stage2-validation.json): **COMPLETE**, 16/16 con USB y 16/16 en batería, ocho por dirección/fase, ventana 5 s; sin duplicados/tardíos/errores nuevos. Idle/profiler, guardas aisladas, GATT, uptime, cuatro contadores MCU y regreso USB/originales aprobados |

El [historial y decisión P1](../out/easyskymesh-stage3-2026-11-09/pm-stage1-trial-history-and-promotion.json), SHA256 `fb1428cbc92404b46a804eaa10cc96eef50ae85a5b4a993172f340fa8918ed89`, mantiene explícitamente `first_failure_remains_failed=true`, `stage1_trials_all_passed=false`, causa **indeterminada** y `stable_sdk_qualified=false`. La repetición no corrige ni recalifica el primer intento. La decisión del principal sólo continúa el experimento aislado P2; no demuestra que el SDK sea estable o que modem sleep haya causado/resuelto la pérdida.

### P2 físico: residencia de modo y coexistencia, sin medición de ahorro

El [resultado completo P2](../out/easyskymesh-stage3-2026-11-09/pm-stage2-attempt-1794276108810686300-a26c12/pm-stage2-validation.json), SHA256 `6f0af6706e2fb7044ccf25b197ddd913b4e7c9a7c850b5184f7ed60a90f58fda`, declara todas las fases y `stage2_qualified=true`. Son 32 contenidos únicos exactos: 16 con USB y 16 con batería, ocho por dirección en cada fase, ventana fija 5 s. Cada radio incrementó TX directo y RX directo en ocho por fase; delta RXerrors fue 0. No hubo duplicados ni tardíos. Las configuraciones RF/range/TX permanecieron iguales.

Después de retirar USB, confirmar COM29 ausente y superar su gracia, el baseline se tomó con configuración 40/80 estable, BLE activo y USB/GPS/OLED libres. Durante el intervalo idle, el profiler sumó **12 854 200 µs nuevos en APB_MIN**, con fila `min_mhz=40`, sobre 20 405 004 µs sumados de modos. Durante RF en batería sumó otros **48 939 098 µs nuevos en APB_MIN**. No se adjudican a residencia nueva valores acumulados antes del baseline. `cpu=80/minObserved=80` en las consultas de tarea es compatible con este resultado: la evidencia de modo idle proviene del profiler SDK fijado, no del mínimo muestreado en la tarea.

En batería, GPS ON aislado retuvo g1/CPU80 con USB/OLED libres; GPS OFF liberó g. OLED ON aislado retuvo o1/CPU80 con USB/GPS libres y OFF liberó o. SPI permaneció positivo/equilibrado, BOOT liberado y errores cfg/fallback/BT/SPI/fault en 0. Las lecturas core/batería superaron la caché compilada de 10 s sin errores; eso ejercita la ruta protegida y no califica eléctricamente el ADC. BLE permaneció conectado, con los cuatro contadores de sueño manual (`sleeps/gpio/timer/ms`) en 0. El uptime avanzó 412 s, coherente con la ventana host. El ciclo previo BLE-off/PRG/reconexión mantuvo la firma GATT.

GPS/custom/display/runtime se restauraron y verificaron **antes de reconectar USB**, con [proof inmutable](../out/easyskymesh-stage3-2026-11-09/pm-stage2-attempt-1794276108810686300-a26c12/settings-restored-before-usb-reconnect.json), SHA256 `6382bfe34631a4c43128d4dcf9394e3b28a55239d040ce66c933313a70e88e88`. Luego el regreso USB acreditó u1/CPU80, metadata/RF/TX originales y cero sueño MCU; los cierres quedaron sin errores. Los checkpoints atómicos originales y el ledger de intentos siguen conservados. No se necesitó atribuir un ACK perdido ni usar una recuperación para declarar restauración.

No hay medidor de corriente. Modem sleep habilitado no equivale a observar la residencia del controlador en cada evento BLE. Estos deltas prueban residencia nueva del modo APB_MIN configurado a 40 MHz bajo las condiciones del ensayo; no mA, energía, autonomía ni equivalencia de sensibilidad. LAB sólo se calificó funcionalmente en Companion BLE: no califica Wi-Fi, sueño manual ESP32 con SDK personalizado, todos los roles, RXPS, Solar/ciclo solar o una distribución estable. El sueño real observado en batería con 64 mensajes pertenece al debug con **SDK normal**, no a LAB. P1 mantiene su pérdida inicial de causa indeterminada y no se promociona el SDK como estable.

## Regreso al firmware normal y RF final

La [transición app-only al normal](../out/easyskymesh-stage3-2026-11-09/controlled-app-transition-attempt-1794276638598572500.json), SHA256 `648fe5a5d8019bb1e2a6d042033c39f5156ea7ea2f6706394ad6fbb840da2b8e`, verificó la imagen anterior P2 exacta y restauró la OTA normal **0eb0e90e… / 1 634 816 bytes**. La aplicación quedó verificada; NVS, OTADATA, app 1, SPIFFS, coredump, bootloader y tabla conservaron sus hashes. No hubo erase completo, respaldo nuevo, exportación de datos o modificación de la referencia. El campo `boot_verified=false` de ese registro significa que la verificación de boot fue deliberadamente separada.

El [preflight posterior](../out/easyskymesh-stage3-2026-11-09/transition-preflight-1794276713039024300-1eff04.json), SHA256 `4e1559a0351d85123f4c5391334aeb53a8ea20e02e4c12b684c974083e3355de`, **sí confirmó boot PASS** de `v1.1.2-tf04db3d3`, identidad y RF/range/TX esperados, GPS request 0/cycle 0/acquire 120/rest 900/hold 5/awake 0, runtime sleep 1 y errores core/BLE/sueño 0. Los originales custom/display estaban verificados al terminar P2 y sus regiones persistentes se conservaron al restaurar; no se presenta esta lectura de boot como un getter independiente de todos esos campos.

El [RF final del normal](../out/easyskymesh-stage3-2026-11-09/rf-final-candidate-attempt-1794276728811996300-9f9d9c.json), SHA256 `4d194ba185aed1041535ab600e6108e616ab80079683336c60d190d1501e61d3`, aprobó **16/16 exactos**, ocho por dirección, ventana 5 s, sin duplicados/tardíos y delta RXerrors 0 en ambos. GPS original 0 y TX/RX/range se conservaron; los contadores directos subieron ocho por sentido/nodo, con tráfico FLOOD ambiental aparte. No se redujo potencia ni perfil RX. Con BLE conectado/USB no hubo sueño MCU; este ensayo no sustituye el ensayo de sueño debug.

Se usó [una copia RF final](../out/easyskymesh-stage3-2026-11-09/two_companion_rf_final.py), SHA256 `ef4046989e785e08d4f07db46a956092c0e688f12f8e8e12b001b11af1112f45`, con **sólo dos líneas de nombres de salida distintas** del programa congelado FE93. AST, `--check-only` y revisión independiente aprobaron; protocolo, payload, condiciones, ventana y validadores no cambiaron. El prefijo `rf-final-` preservó canónicos antiguos y el ensayo normal fallido a 2,5 s permanece FAIL.

El computador cerró sus conexiones BLE. El [registro manual final](../out/easyskymesh-stage3-2026-11-09/final-phone-oled-user-confirmation.json), SHA256 `7783e11cceaded1f60b167755de288b0d934f184963237451eaade92bdb20735`, conserva la respuesta exacta del usuario: **«si todo correcto»**. Confirmó conexión del teléfono sin volver a emparejar, sus datos habituales, lectura clara de PIN y uptime al despertar con PRG y apagado OLED aproximado a los 15 s. Es confirmación explícita del usuario, no una observación automática ni una medición cronometrada; el PIN no se guardó.

El cierre conjunto diferencia las fuentes: GPS completo y permiso runtime se releyeron en normal; custom/display completos se verificaron antes de volver a normal, se conservaron NVS/SPIFFS durante la transición y la pantalla se confirmó manualmente. **No hubo un segundo getter normal de CMD40/display** y no se afirma que lo hubiera. El `boot_verified=false` del registro ROM original y los pendientes de los componentes debug son estados históricos superados por evidencias posteriores; sus archivos permanecen sin editar.

## Disposición de las recomendaciones originales

| Recomendación original | Disposición concreta |
|---|---|
| PATH bounds y cola RX/TX pendiente | Integrados y validados en etapas anteriores; conservados en f04. No se demostró una explotación ni una pérdida previa atribuible |
| Reset/pines/OLED uptime y estado | Integrados en `ac19a969` y conservados. Companion físico confirmado en etapa anterior; indicadores Repeater nuevos sólo compilados/revisados entonces |
| CAD diagnóstico | Integrado en f04; tests del wrapper real con frontera radio. No se activó CAD ni se calificó ocupación física |
| Lectura serie ocupada antes de reposo | Integrada en f04 y cubierta en host. Batería 64 demuestra sueño/RX, pero no valida físicamente fragmentación de cada transporte ni reemplaza los tests host |
| GPS por ciclos opt-in y paridad de comandos | Integrados en f04; host, estados/UART y 32 RX durante descanso GPS en batería aprobados. Fix fresco/posición/UTC físico no obtenidos |
| Ahorro con BLE activo | Sólo LAB:66 host, P0 completo; P1 FAIL7/8 conservado y repetición completa 16/16; P2 completo 32/32 y residencia APB_MIN40 observada. SDK no declarado estable ni integrado/predeterminado en normal |
| RXPS por defecto y temporizador SX1262 experimental | No adoptados: no corresponden a RX continuo ni satisfacen el requisito de conservar escucha LoRa. Un experimento RXPS futuro necesitaría otra calificación completa |
| LNA apagado/bajar TX predeterminado | No adoptados: conservar range/LNA y objetivo nominal máximo por FEM. No se rebajó para aprobar pruebas |
| TCXO global 1600 µs | No adoptado sin medir Heltec V4/condiciones frías. El contador CAD no cambia este tiempo |
| Sustituir bucles de sueño externos de 5/30 s | No adoptado: se conservan mantenimiento/guards y sueño manual acotado 20 ms |
| Ampliar motor 20 ms a Room Server/Sensor | Adaptación compatible futura explícita, no implementada ni ensayada en esta etapa. Necesita política de colas, temporizadores, wake/servicio y pruebas específicas por rol; no se deduce de compilar sus imágenes |
| Sustituir persistencia, perfiles, ANON/LPP o Wi-Fi secreto | No adoptado: se conservan guardas transaccionales/críticas, separación Solar/Companion, límites y consultas sin devolver credenciales |
| RTC adicional nRF52 y diagnóstico M6/solar | No aportan equivalente nuevo para esta placa. Se conserva RTC ESP32 propio; no se inventa detección física de cargador/solar |
| Reinicio periódico opcional | No incorporado como remedio de recepción: interrumpe servicio y exige política de drenado/guardado/roles. No hay necesidad concreta demostrada |
| INA228 | Pospuesto hasta disponer del componente/shunt y un caso real. Si se adapta, corregir Wh→kWh y probar unidades; añadir código sin hardware no mide este nodo |
| Dev-PS18 outpath/login/ACL | Pospuesto por sentinel 0xFE, respuesta de login DIRECT/FLOOD y compatibilidad persistencia/downgrade. Requiere diseño separado y pruebas integradas; no es una omisión accidental |
| Nuevas variantes/placas/TFT/R8 | No se reintrodujeron. Los cambios compatibles continúan dentro de los diez roles mantenidos |

El inventario diferencia cambios priorizados implementados, experimento aislado y adaptaciones futuras con decisión explícita. No afirma que toda función externa esté incorporada ni que Room Server/Sensor ya tengan el motor 20 ms. INA228, reboot y outpath mantienen sus límites concretos; no son pasos silenciosamente omitidos ni requisitos automáticos para aprobar Companion/Solar actual.

## Alcance cerrado y límites conservados

El cierre conjunto es PASS para la implementación compatible priorizada, las pruebas funcionales descritas y el regreso verificado a normal con teléfono/OLED/PRG confirmados. No significa que todos los intentos aprobaran: el fallo normal de 2,5 s y el primer P1 7/8 siguen siendo FAIL y no se identificó su causa. Tampoco significa que todas las opciones externas se incorporaran; el inventario anterior mantiene sus decisiones y adaptaciones futuras explícitas.

No se calificaron corriente/autonomía, potencia RF o sensibilidad/enlace medidos, fix GPS/UTC exterior, tres radios/enrutamiento autenticado controlado, soak 48 h, ciclo solar, Wi-Fi o sueño manual con SDK LAB, fragmentación física de todos los transportes, nueva UI Repeater física o motor 20 ms Room/Sensor. El usuario no podía mover el GPS a cielo abierto; no se inventó un fix para cerrar el ensayo. El SDK PM continúa experimental y no está en el normal restaurado.

No hay hardware pendiente de este plan de validación acotado. Esta documentación y su resumen no integran firmware a main ni publican un release. Los cortes históricos, hashes y pruebas originales se conservan en el índice final; las imágenes normales de f04 mantienen sus mismos bytes.
