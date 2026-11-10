# Validación de la integración selectiva de EasySkyMesh para Heltec V4

**9 de noviembre de 2026. Integración selectiva de prioridad inicial validada.** Ambas etapas tienen pruebas nativas y matriz completas. La segunda aplicación está instalada y arrancada, aprobó el ensayo RF y el usuario confirmó OLED, PRG, autoapagado y conexión del teléfono. Los resultados corresponden al firmware `ac19a969`; `main` y el release estable continúan en `62e184a6`.

## Alcance y fuentes

Se adaptaron funciones concretas sobre nuestro fork, conservando su radio, potencia máxima por FEM, sueño acotado, GATT estable, reconexión BLE, USB, PRG/OLED, GPS opcional y persistencia. No se fusionó la rama completa PowerSaving-v18.

| Referencia | Commit o versión |
| --- | --- |
| Base propia, `main` y estable v1.1.2 | `62e184a6f104a0f75bb5054e64471275c553b6e8` |
| Comparación previa de funciones y riesgos | `99a20200d1899fccfa5192dc7a8365aaf520b5b1` |
| Bounds del plaintext PATH recibido | `7b7e09f891bba665b79e3bb4bc40a06bb8ab7631` |
| Trabajo pendiente RX/TX antes de dormir | `53997a90122055d96312a3533cc10c49476c7289` |
| Diagnósticos de sólo lectura e indicadores OLED | `ac19a969e3ae3cf43dea991545b9a1d73cb91188` |
| Candidato local de primera etapa | `v1.1.2-t53997a90` |
| Candidato local de segunda etapa | `v1.1.2-tac19a969` |
| Código externo estable examinado | `IoTThinks/MeshCore`, `PowerSaving-v18`, `17e062ff6a1518740aebc120101ae024513a8832` |

La rama de trabajo es `integrate/easyskymesh-selective`. Las versiones con sufijo temporal identifican imágenes de validación local; no son nuevos releases estables. Las diferencias entre la rama externa estable y `Dev-PS18` se documentaron por separado en la comparación inicial.

## Cambios de la primera etapa

### Plaintext PATH

`PeerPathPayload.h` valida la longitud real descifrada, la codificación de la longitud de ruta y que quepan ruta y byte de tipo adicional antes de acceder a esos campos. `Mesh.cpp` rechaza el plaintext inválido antes del callback. Se conservan los modos de hash de ruta de 1/2/3 bytes y el tratamiento del padding existente.

Esta mejora no activa RXPS, no cambia rutas de contactos por iniciativa propia y no adopta los nuevos sentinelas de política de ruta de `Dev-PS18`.

### Trabajo pendiente antes del reposo

`PacketManager::getInboundTotal()` y `StaticPoolPacketManager` exponen toda la cola entrante, incluso paquetes diferidos. `Dispatcher::isTransmitting()` cubre el paquete que ya salió de la cola pero sigue en un envío asíncrono. Companion y Repeater incorporan esas consultas a `hasPendingWork()`, conservando la escritura diferida de contactos y la exclusión del puente activo.

Los tests utilizan la cola y el Dispatcher reales: plazos futuros, drenado, operaciones nulas, independencia de colas, desbordamiento de 32 bits y finalización/fallo/timeout de TX. No se duplica la expresión de producción en un helper de prueba.

El motor de sueño de 20 ms y las guardas de RX, USB, pantalla, GPS y BLE permanecen iguales. Un paquete entrante diferido puede mantener despierto el ESP32 durante su espera, hasta el límite existente de 32 s; es una consecuencia deliberada de la decisión más completa de reposo. No se ha demostrado que la diferencia anterior provocara pérdidas de paquetes.

## Segunda etapa: observabilidad y pantalla

El commit `ac19a969` añade:

- `get power.reset`: motivo ESP32 en número y descripción.
- `get sensor.pins`: asignaciones compiladas de I2C y GPS, disponibilidad del driver y presencia GPS marcada `unverified`.
- Uptime de 64 bits en el inicio OLED de 128×64 mediante `esp_timer_get_time()`, evitando depender del desbordamiento de `millis()`.
- Voltaje y estado lógico de PowerSaving en la pantalla de Repeater, usando el getter de batería con caché existente.

Los getters no sondean dispositivos, no inician UART, no cambian raíles y no toman muestras ADC. `gps-driver:compiled` **no confirma que exista físicamente un GPS**; se conserva explícitamente `gps-detected:unverified`.

Los comandos nuevos usan el protocolo Companion existente CMD66 RUN_CLI_COMMAND y RESP29. CMD40 GET_CUSTOM_VARS enumera ajustes del gestor de sensores y no es el transporte de estos getters. Las respuestas de los nuevos handlers se acotan a 160 bytes de buffer; el informe de pines del BLE OLED normal tiene 119 caracteres.

La presentación compacta se limita a Heltec OLED de 128×64 sin el modo de reloj alternativo. Conserva PIN en tamaño 2, situado en y=38; IP en y=29 y uptime en y=56. La actualización visual se ejecuta cuando la pantalla ya está encendida y no renueva su autoapagado. La lectura de batería puede refrescar el ADC al caducar la caché, pero no fuerza una conversión en cada frame.

La revisión estática de estos cambios no encontró alteraciones de RF, TX, sueño, alimentación GPS o VEXT. La aplicación también aprobó los ensayos de arranque y RF descritos abajo. El usuario confirmó la legibilidad y el comportamiento visual del Companion BLE. Los indicadores nuevos de Repeater se compilaron y revisaron estáticamente; su pantalla no se probó físicamente en esta etapa.

## Plataforma y condiciones de la prueba

Se utilizó un Heltec V4 de alta potencia como DUT y un MuziWorks SuperBase como referencia Companion. El firmware de la referencia `v1.18-sb2-rc1` permaneció intacto; no se escribió firmware ni configuración en ella durante los intercambios RF registrados.

El dispositivo llamado MeshMonitor es **otro equipo** y está excluido de esta validación. Las herramientas de la segunda etapa fijan sólo DUT y MuziWorks autorizados, sin selección por nombre ni opciones para conectar otro equipo. Este informe omite direcciones de dispositivos, PIN, hashes privados de identidad y ubicaciones.

La RF compartida registrada fue 910.525 MHz, BW 62.5 kHz, SF 7 y CR 5. El DUT conservó perfil `range`, boostedRX interno y FEM/LNA activo. Su diagnóstico mostró solicitud SX1262 de 22 dBm, entrada aplicada 21 dBm y salida **estimada** 28 dBm mediante la tabla del FEM. No se midió potencia RF radiada ni conducida.

## Pruebas realizadas: primera etapa

| Comprobación | Resultado respaldado por registros |
| --- | --- |
| Suites nativas para `53997a90` | 106/106 PASS, 34.713 s |
| Matriz de compilación | 10/10 roles PASS; 20 imágenes, OTA y merged por rol |
| Instalación de la app candidata | Imagen y arranque de `v1.1.2-t53997a90` verificados |
| Estable: primer intento | 13 probes PASS de 14 intentados; 16 planeados. Intento completo conservado comoFAIL |
| Estable: segundo intento | 16/16 PASS, 8 en cada dirección |
| Candidato `53997a90` | 64/64 PASS, 32 en cada dirección |

La matriz contiene los diez roles que ya mantenía la distribución: Solar Repeater, Expansion Kit Repeater, puente ESP-NOW, Room Server, Terminal Chat, Companion USB, Companion BLE, Companion WiFi, Sensor y KISS. Las 20 imágenes no representan 20 variantes.

### Intento estable fallido conservado

El primer ensayo planeaba 8 envíos por dirección. Se realizaron 8 DUT→referencia con éxito y 6 referencia→DUT, de los que 5 tuvieron entrega válida. El programa se detuvo en `probe_reference_6`; quedaron 2 probes sin intentar. El resultado fue 13/14 intentados, no un resultado aprobatorio de 13/16.

No se registraron nuevos errores RX en los counters, pero ese dato no identifica la causa de la falta de entrega. El intento conserva sus contadores y el fallo original. La causa permanece **indeterminada**.

Después se separaron los equipos y se retiró el USB de la referencia. El siguiente ensayo estable dio 16/16. Ambas condiciones cambiaron antes de repetir y no hubo un experimento aislado por factor: **no se atribuye causalidad** a distancia, USB, interferencia o saturación del receptor.

### Resultado RF del candidato

Los 64 probes de `53997a90` recibieron exactamente un PUSHRAW coincidente dentro de la ventana, sin duplicados ni coincidencias tardías en el registro del ensayo. En cada nodo aumentaron 32 las transmisiones directas y 32 las recepciones directas, con delta de errores RX 0. También se contabilizó tráfico ambiental: 1 FLOOD adicional en DUT y 2 en referencia.

La referencia tenía 180 errores RX históricos antes de esta campaña; no se borraron. La aceptación del ensayo depende de que el incremento durante su ventana sea 0, no de exigir un contador histórico absoluto 0.

Identidad, versión y RF se compararon antes y después mediante metadatos filtrados. Los errores core quedaron en 0 y el uptime fue continuo dentro del ensayo. Los diagnósticos conservaron TX máximo estimado y recepción `range` con LNA/boostedRX.

## Instalación y conservación de datos

La primera instalación escribió únicamente la aplicación del slot activo, sin borrado completo ni nuevos respaldos del dispositivo. Los datos de NVS, OTADATA, SPIFFS y coredump, además del bootloader y la tabla de particiones, se compararon por MD5 dentro del equipo antes y después; el registro confirma que no cambiaron. No se exportó su contenido para realizar esa comparación.

La segunda instalación exigió manifiesto final e imagen con SHA256 fijados, inventario 10/20, código y versión esperados, preflight reciente y el resultado RF 64/64 de la primera etapa. Verificó chip/eFuse autorizado, tabla y OTADATA reales, slot y aplicación anterior exactos antes de escribir. Se instaló únicamente la app BLE de `ac19a969`, de 1.629.216 bytes. Quedó verificada y se comprobó que app1, NVS, OTADATA, SPIFFS, coredump, bootloader y tabla de particiones conservaron sus digests.

`application_verified` y `boot_verified` son estados separados. El archivo original `observability-install-attempt-1794268594523618900.json` permanece inmutable con aplicación verificada y arranque aún pendiente en ese momento. Después se leyó por BLE la versión exacta del firmware en ejecución y se completó RF 16/16; esa evidencia permitió crear `candidate-installation.json` con `boot_verified:true`. No se reescribió el intento original para atribuirle una comprobación posterior. Un reset de ROM por sí solo no se contó como prueba de arranque.

## Qué prueban los intercambios RAW y qué queda fuera

La prueba utiliza CMD65 con RAW_CUSTOM directo de cero saltos y payload sintético de 32 bytes, nonce nuevo y registro de coincidencia exacta PUSH0x84. La aceptación del comando sólo prueba encolado; se exigen además TX/RX, recepción y checks de errores. No se usan mensajes públicos o privados de usuario, FIFO, contactos, canales, login ni claves.

Ambos enlaces BLE permanecen conectados. Esto verifica RF y entrega a interfaz **con BLE activo**; nuestras guardas impiden sueño real ESP32 en esas condiciones. RAW offline no queda en la cola de mensajes, así que este ensayo no acredita recepción correlacionada durante suspensión del controlador.

RAW_CUSTOM no autentica la identidad del emisor ni cifra el contenido de prueba. «Sin duplicados» se refiere a entregas observadas por la interfaz; la tabla seen puede suprimir duplicados recibidos por radio antes del PUSH.

Los probes RAW no atraviesan el decoder de PATH autenticado. Esa rama exige contacto y MAC/secreto válido antes de descifrar; su nueva validación de plaintext tiene evidencia nativa. No se afirma haber probado PATH cifrado real en los nodos ni se añadieron contactos para ello.

Tampoco se han medido corriente, autonomía, sensibilidad, cobertura, potencia RF o equivalencia estadística de entrega. Los resultados 16/16 y 64/64 son muestras de esta campaña, no una garantía de pérdida 0 en cualquier entorno.

## Estado de la segunda etapa

| Comprobación | Estado al redactar |
| --- | --- |
| Suites nativas `ac19a969` | 106/106 PASS, 33.193 s |
| Matriz de diez roles | 10/10 PASS; manifiesto `complete:true`, `pass_all_targets:true`, fuentes limpias al terminar |
| Total de imágenes y SHA256 final | 20 imágenes; tabla de checksums de 20 entradas y hashes indicados abajo |
| Instalación sólo de la aplicación | App verificada; todas las regiones protegidas, incluida app1, conservaron sus digests; sin respaldo ni borrado completo |
| Arranque de la app `v1.1.2-tac19a969` | PASS mediante lectura BLE de versión/identidad/RF y prueba posterior de tráfico |
| Getters nuevos por BLE, respuesta completa | PASS antes/después: reset válido, pines esperados, driver compilado y presencia `unverified` |
| RAW bidireccional, 8 por dirección | 16/16 PASS, sin duplicados, tardíos, entregas al extremo incorrecto ni nuevos errores |
| OLED físico: PIN y uptime legibles | PASS, confirmado por el usuario: PIN grande y uptime legibles sin solapamiento |
| PRG, ventana de visibilidad y autoapagado | PASS, confirmado por el usuario: restauración con PRG y autoapagado aproximadamente a los 15 s |
| Reconexión y uso normal con teléfono | PASS, confirmado por el usuario: conexión sin volver a emparejar y datos habituales presentes |

La confirmación manual se conserva en `manual-validation.json`. Abarca únicamente el Heltec de pruebas en Companion BLE: PRG, PIN/uptime, autoapagado y uso desde el teléfono. No se pidió ni registró el PIN.

El ensayo real de segunda etapa produjo exactamente 8 incrementos TX directo y 8 RX directo en cada nodo; los errores core quedaron en 0 y el delta de errores RX fue 0. La referencia mantuvo sus 180 errores históricos. Todos los checks del JSON aprobaron, con identidad, versión, RF, TX y perfil RX conservados antes y después. Los contadores de sueño del DUT permanecieron en 0 con BLE conectado; eso es coherente con la guarda y no constituye un ensayo del sueño real.

Los getters nuevos reportaron antes y después el motivo de reset 0, `Unknown or first boot`, sin inferir una causa adicional. Los pines compilados fueron I2C principal SDA17/SCL18, bus ambiental -1/-1 y GPS RX38/TX39/EN34. El driver quedó `compiled` y la presencia `unverified`, sin sondeo físico de GPS.

El programa de segunda etapa validó sólo las dos direcciones autorizadas, usó colas de notificaciones limitadas a 1024 y exigió MTU suficiente para las respuestas nuevas. Rechaza pines incorrectos, presencia GPS afirmada o reply truncado. Importarlo y usar `--check-only` no cargan BLE ni abren puertos.

Un fallo posterior debe conservarse como intento independiente. No se deben sustituir fallos por un canonical PASS ni convertir checks offline del programa en evidencia de radio o pantalla.

## Evidencia local y hashes

Los registros siguientes permanecen en `out/`, ignorado por Git; no son assets públicos del release. Se citan sus nombres sin incluir datos privados del dispositivo. Los hashes identifican archivos de validación o firmware, no identidades personales del nodo.

| Evidencia de primera etapa | SHA256 |
| --- | --- |
| `easyskymesh-integration-2026-11-09/candidate-build-results.json` | `0ea9a265ac63db6f07d0812de77e2c3edf6b444056abff1aac5ea09403152510` |
| `easyskymesh-integration-2026-11-09/candidate-SHA256SUMS.txt` | `43cdd383ece9a7dea1db6ada448ce2e6f89ef0b749857ee352329f11cf30dcc3` |
| `easyskymesh-integration-2026-11-09/native-tests-initial.log` | `b28f2de720d98fbeed7e21ae5ef2082d596c49fbf8f11ff2fad93f5dc2a79882` |
| `rf-stable-attempt-1794264979035243200-41c4ce.json`, FAIL conservado | `f301fa61e44cae66b95376c7b7ba01c5db5712e0f9b1f3d9bbafe20c1941ace3` |
| `rf-stable-baseline.json`, 16/16 PASS | `a99b08f3dc1b7823efb38a6e3de56c414e86c0c46a31baf7eae84cf5058f0e31` |
| `rf-candidate.json`, 64/64 PASS | `89273fa8822a309f3d8303ffb9973dc55284e019bb84c57014a04d3799697e71` |
| App BLE candidata `v1.1.2-t53997a90` | `f36e76d600382c29ebd32bcb4e519b3c6349116f4aa48af5cc25325a218f38a2` |

Los tres archivos RF de la tabla están en `out/easyskymesh-integration-2026-11-09/`. `candidate-installation.json` conserva la verificación de la aplicación, arranque y regiones de la primera etapa. El resultado 16/16 también se conserva en su archivo de intento individual.

| Evidencia de segunda etapa | Estado/hash |
| --- | --- |
| `easyskymesh-observability-2026-11-09/native-tests.log` | `b07623eb679be02fbf9a5145809f261decca026a2eb2109a99e29d19fcdac0a5` |
| `candidate-build-results.json` final | `cc50a95f4750a28ea7eceffedb9a7c3376c2a466a67755c545427e2406212f85` |
| `candidate-SHA256SUMS.txt`, 20 entradas | `8df08d2fc576dc2d4d471520394e4cb8ece9ba380f65387bed3c234d1901e4ff` |
| App BLE `v1.1.2-tac19a969`, 1.629.216 bytes | `83fcd0ced0ef1c26cd13a557d6d5111389ec78fe5776c41a27376ce04f55c85d` |
| `observability-install-attempt-1794268594523618900.json`, original inmutable | `c6a14499baf4556f2e4ad28cc79a795375f2abdf5f2b789cfff949bc5bd17088` |
| `candidate-installation.json`, verificación posterior de arranque | `760b6293d50076e8f1ea75c68caeb6025273e238cafdf4a77a4f4f05a22cc8e5` |
| `rf-candidate.json`, 16/16 PASS | `4704b5ffb5c46bf251a76e2115a6773b3696cb09631f4833d389f03238fa75b7` |
| `manual-validation.json`, confirmación del usuario | `d22703bd9c2b2e61070852bea4b107cbb4393263872db2061826da95e0e9fe81` |

Los archivos de esta segunda tabla están en `out/easyskymesh-observability-2026-11-09/`. El RF aprobado se conserva también en el intento único `rf-candidate-attempt-1794268599226844200-1f9783.json`, con el mismo SHA256 que su canonical. `inherited-baselines.json` identifica las copias de metadatos aprobados de la primera etapa: no son respaldos nuevos del nodo. `transport-review-and-run.md` conserva la revisión del transporte y las órdenes; los resultados físicos están en sus JSON de ejecución, no se deducen únicamente del código del programa.

## Trabajo futuro separado y estado de publicación

Contador CAD, GPS por ciclos, framework/DFS con BLE activo, INA228, políticas de rutas y RXPS son temas separados. No se añadieron silenciosamente a esta etapa, no se habilitó RX discontinuo y no se presentan como funciones ya validadas. La condición actual de RX continuo y TX máximo estimado se mantiene.

No se ha integrado esta rama a `main` ni se ha lanzado un nuevo release. La documentación de resultados se prepara después de la matriz y no modifica las imágenes ya verificadas de `ac19a969`; un commit futuro que cambie firmware necesitará su propia evidencia pertinente.
