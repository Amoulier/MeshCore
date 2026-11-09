# Comparación de EasySkyMesh con nuestro fork Heltec V4

Fecha de revisión: 9 de noviembre de 2026, America/La_Paz.

## Dictamen

Conviene aprovechar aportes concretos de EasySkyMesh mediante adaptaciones pequeñas. No conviene fusionar su rama completa ni sustituir nuestros archivos de placa, Bluetooth, radio o almacenamiento.

Los primeros candidatos son una comprobación más completa del trabajo pendiente antes de dormir, diagnóstico de pines y reinicios, e información de estado en la pantalla. El GPS por ciclos merece una segunda etapa, conservando siempre el apagado decidido por el usuario. El ahorro con Bluetooth activo requiere investigar un cambio del framework y validarlo por separado. RXPS y el apagado del LNA no son valores predeterminados adecuados para nuestro requisito de conservar la recepción LoRa. El sueño profundo para telemetría del sensor aparece como hoja de ruta externa, no como una función terminada que debamos portar.

Esta revisión no incorpora cambios al firmware ni actualiza nodos. Se crean este informe y copias aisladas para análisis y pruebas de ordenador. Las recomendaciones de compatibilidad son condiciones de diseño y validación; no equivalen a una integración ya probada físicamente.

## Fuentes fijadas y alcance

| Componente | Referencia analizada |
| --- | --- |
| Nuestro `main`, igual a la release Heltec v1.1.2 | `62e184a6f104a0f75bb5054e64471275c553b6e8` |
| EasySkyMesh, repositorio de documentación y distribución | `4f3720c8252d3fe77cafee9b786ff4208723ac83` |
| Código de firmware publicado como PowerSaving 18 | `IoTThinks/MeshCore`, `PowerSaving-v18`, `17e062ff6a1518740aebc120101ae024513a8832` |
| Base oficial de esa rama PowerSaving 18 | `81dbae592ffad40b4212bc0b37e82b38da08f4bc`, MeshCore 1.18.0 |
| Rama adicional de desarrollo, evaluada por separado | `Dev-PS18`, `b568b526fb00be4b802beb3cbf45d580fc480577` |
| Ancestro común de nuestro fork y PowerSaving-v18 | `65aa1138ae0506703cab74d5e94bb924bd4516b4` |

EasySkyMesh enlaza expresamente al segundo repositorio para el código fuente. Su release PowerSaving18 se publicó el 22 de octubre; la rama enlazada siguió recibiendo cambios hasta el 9 de noviembre. No se supone que todos los binarios publicados anteriormente contengan este último código. También se revisó `Dev-PS18`: sus novedades no deben presentarse como parte de la rama estable enlazada en la release.

Se inventariaron las diferencias y se revisaron los componentes relevantes para Heltec V4 OLED: radio SX1262/FEM, ESP32-S3, BLE/USB/Wi-Fi, gestión de colas, aplicaciones, GPS, sensores, pantalla, configuración, protocolos y compilación. Las implementaciones exclusivas de otras placas se clasificaron como fuera del alcance de nuestro firmware; no se afirma haber validado esas placas.

La rama PowerSaving-v18 tiene 49 commits posteriores a su base oficial, que afectan 152 rutas en todo su proyecto; 66 rutas corresponden a `src`, aplicaciones, Heltec V4, pruebas y configuración principal. Entre los dos árboles completos hay 875 rutas diferentes, muchas por nuestra retirada deliberada de otras placas y documentación. Esas cifras no representan 875 funciones nuevas.

Una simulación de fusión con `git merge-tree` produjo conflictos en 152 rutas, incluidos los archivos de Companion, preferencias, radio, BLE, GPS y Heltec V4. La simulación se hizo en una copia de análisis y no aplicó la fusión al proyecto. Además de esos conflictos textuales, hay incompatibilidades de comportamiento que Git no detectaría.

## Inventario de funciones y recomendación

| Función externa | Situación en nuestro fork | Conveniencia |
| --- | --- | --- |
| Dormir el ESP32 manteniendo LoRa en RX | Ya está en BLE y Solar, con ventanas de hasta 20 ms y protecciones propias | Conservar nuestra implementación |
| Detectar cola entrante diferida y transmisión activa antes de dormir | Nuestra comprobación de trabajo pendiente es menos completa | Adaptar con prioridad |
| Estado de lectura pendiente en interfaces (`isReadBusy`) | No existe esa interfaz común | Adaptar de forma acotada; no copiar métodos que devuelven siempre `false` |
| Contador de canal ocupado por CAD | Falta ese contador específico | Adaptar como diagnóstico opcional, sin importar RXPS |
| GPS con ventanas de adquisición y descanso | Tenemos apagado real, UART detenida y controles; falta el planificador cíclico | Adaptar como opción, después de pruebas |
| CLI de Companion para GPS y ahorro persistente | Tenemos controles de diagnóstico y GPS mediante la app; falta paridad textual y parte de la persistencia | Añadir sólo los controles útiles, conservando nuestras políticas |
| Tiempo encendido en OLED | Ya se informa uptime al cliente, pero no se muestra igual en la pantalla | Adaptar el indicador visual |
| Voltaje y estado de ahorro en OLED de Repeater | Hay diagnóstico propio; falta parte de esa presentación visual | Adaptar sin despertar la pantalla ni aumentar el muestreo |
| Comando de inventario de pines `sensor` | Tenemos `sensor list/get/set`, pero no ese inventario resumido | Adaptar con sintaxis inequívoca |
| `powerlog` con motivo de reinicio | El ESP32 ya proporciona el dato; falta ese acceso CLI sencillo | Adaptación pequeña y útil |
| CPU dinámica y ahorro con BLE activo | Hay infraestructura condicional, pero el framework estable no habilita PM | Prototipo separado; no sustituir el framework estable directamente |
| RXPS: escuchar LoRa por ciclos | No está; mantenemos recepción continua | No habilitar por defecto; sólo posible ensayo futuro explícito |
| Desactivar FEM/LNA para ahorrar | Ya tenemos controles y perfiles RX; LNA activado por defecto | No adoptar el valor apagado predeterminado |
| Potencia inicial de 17 dBm en SX1262 | Tenemos bloqueo de máximo físico y mapeo según FEM | No sustituir nuestra política |
| Ahorro en Room Server y Sensor | No tienen el mismo sueño explícito de BLE/Solar | Posible adaptación futura de nuestro motor de 20 ms, no sus pausas largas |
| Reinicio periódico (`reboot.interval`) | No existe | Baja prioridad; no usar como sustituto de corregir fallos |
| Hora conservada entre reinicios | Nuestro ESP32-S3 ya tiene respaldo de hora en RAM RTC; la aportación nueva se dirige a nRF52 | Ya cubierto para Heltec dentro de los límites de retención RTC |
| Reducir establecimiento TCXO a 1.600 µs | No usamos esa reducción global | No adoptar sin medir nuestra placa; su motivación principal es RXPS |
| BLE Companion para Expansion Kit y buzzer | Tenemos kit en roles concretos, no esa variante adicional | Sólo si existe necesidad de ese hardware; conservar los diez roles actuales |
| Sensores I2C y detección 0x76/0x77 | Ya tenemos catálogo, descubrimiento y dependencias por rol | No son novedades pendientes generales |
| Nuevas rutas de respuesta y rutas observadas tras login | Sólo aparecen en `Dev-PS18`; no están en nuestro fork | Posponer hasta corregir incompatibilidades y probar la integración |
| INA228 con energía acumulada | Sólo añadido en `Dev-PS18`; no está en nuestro fork | Útil con sensor físico, corrigiendo unidades y calibración |
| Corrección experimental del temporizador RXPS SX1262 | Sólo está en `Dev-PS18` | Depende de adoptar RXPS; no corrige nuestra recepción continua |
| ACK directo, validaciones serie y descubrimiento de nodos ocultos | Las correcciones relevantes comprobadas ya están integradas | Evitar duplicarlas o perder nuestras validaciones posteriores |
| RS232/Ethernet y soporte de otras familias | Fuera de nuestra distribución Heltec OLED actual | No reintroducir por una fusión global |

## Adaptaciones que sí recomiendo

### 1. Completar la decisión de dormir

La implementación externa incorpora `PacketManager::getInboundTotal()`, su implementación en `StaticPoolPacketManager` y una consulta a la transmisión activa. Sus `hasPendingWork()` consideran paquetes entrantes diferidos, salientes y trabajo de calibración. Nuestro Repeater comprueba la cola saliente y el puente; el Companion comprueba guardado diferido de contactos y cola saliente. Nuestra condición de estar en RX ya protege parte de la transmisión, pero no hace explícita la cola entrante diferida.

El beneficio consiste en evitar dormir mientras un paquete espera su procesamiento o hay un trabajo inmediato. No se ha demostrado una pérdida de paquetes en nuestro firmware por esta diferencia: con un temporizador de 20 ms, el riesgo directo es introducir retraso y hacer menos precisa la decisión de reposo.

Adaptación: añadir observabilidad de la cola entrante y, donde corresponda, del TX y de interfaces; conservar la exclusión del puente ESP-NOW. No añadir las dependencias RXPS para obtener esta mejora. En interfaces serie, considerar también un frame recibido parcialmente; la implementación externa de ArduinoSerial devuelve `false`, por lo que no resuelve ese caso por sí sola.

Pruebas: cola entrante diferida sin salientes, TX en curso, frame fragmentado, drenado de colas, reposo posterior y temporizadores próximos. Mantener los límites de latencia y la ventana máxima actuales.

Fuentes: [interfaces y cola](https://github.com/IoTThinks/MeshCore/commit/c1ab0de8), [Repeater](https://github.com/IoTThinks/MeshCore/commit/60e9e410), [Room Server](https://github.com/IoTThinks/MeshCore/commit/af23f950), [nuestro hasPendingWork](https://github.com/Amoulier/MeshCore/blob/62e184a6f104a0f75bb5054e64471275c553b6e8/examples/simple_repeater/MyMesh.cpp#L1343).

### 2. Diagnóstico e información visual

Son cambios de utilidad inmediata y riesgo menor que reemplazar motores de energía:

- Mostrar uptime en OLED, usando un contador que tolere el desbordamiento de `millis()` y mantenga su significado tras el sueño.
- Mostrar estado de ahorro y voltaje usando nuestra lectura de batería ya almacenada en caché.
- Exponer el motivo de reinicio mediante un comando sencillo; el ESP32 ya implementa esa consulta.
- Mostrar los pines de sensores/GPS realmente compilados. El Expansion Kit utiliza su bus específico; no copiar sin más las conexiones genéricas de la guía externa.

Estos cambios deben respetar el apagado persistente del OLED, VEXT, PRG y su intervalo de visibilidad: Solar/Repeater usa 20 segundos y Companion tiene actualmente 15 segundos por defecto. Actualizar un indicador no debe encender la pantalla, reactivar GPS ni añadir conversiones ADC en cada iteración. No reemplazar nuestro porcentaje de batería suavizado por la interpolación lineal de la UI externa.

El comando externo `power` añade varias mediciones de alimentación que el Heltec externo no implementa: hereda respuestas no disponibles. Nuestro `get power.status`, `get power.runtime` y `get power.sleep` ya ofrecen datos pertinentes. No recomiendo copiar esa interfaz para aparentar mediciones USB/solar que la placa no entrega.

Fuentes: [UI Companion](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/examples/companion_radio/ui-new/UITask.cpp#L189), [UI Repeater](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/examples/simple_repeater/UITask.cpp#L109), [CLI](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/src/helpers/CommonCLI.cpp#L509).

### 3. GPS por ciclos, como opción explícita

La aportación es una máquina de estados que distingue GPS autorizado, encendido durante adquisición y dormido temporalmente. Repeater/Room/Sensor solicitan ventanas máximas de 600 segundos con descanso diario; Companion solicita 600 segundos con descanso de 1.800 segundos en el código revisado. Estos valores difieren de parte de la wiki. La adquisición puede terminar antes cuando se completa la sincronización.

Es útil para una instalación que realmente utilice GPS. Si GPS está apagado, el beneficio adicional es cero; no debemos encenderlo para conseguir un ahorro que ya obtenemos manteniéndolo apagado.

Se identificaron problemas concretos que impiden copiarlo literalmente:

- **Ahorro ON→OFF mientras GPS duerme:** la máquina deja `gps_active=true` y `gps_wake=false`, corta la alimentación y posteriormente omite su propia reconciliación al desactivar el ahorro. Se vuelve a ejecutar el parser, pero no `begin()` para encender el receptor. Hay que resolver expresamente esa transición.
- **Telemetría con efectos laterales:** una petición local de telemetría fuerza `syncTime()` cuando hay ahorro GPS. Si el GPS está habilitado, consultar batería frecuentemente puede interrumpir su descanso sin que se haya solicitado una posición nueva. El bloqueo por `gps_active` sí evita que esto encienda automáticamente un GPS expresamente apagado.
- **Fecha límite fija:** `MicroNMEALocationProvider::isValid()` exige año menor que 2030. No adoptar esa caducidad como sustituto de validar la fecha.
- **Estado lógico frente a estado eléctrico:** nuestra condición actual de sueño usa `!gps_enabled`; añadir sólo el planificador no permitiría dormir el ESP32 durante el descanso de un GPS habilitado. La adaptación necesita estados separados y control de UART/rail.

Condiciones de adaptación:

1. `gps off` continúa significando apagado completo: sin sondeo, UART, reset ni despertares por una solicitud de telemetría.
2. Configuración independiente para adquisición, tiempo máximo despierto y descanso; validación de rangos y temporizadores frente al desbordamiento.
3. Mantener el último dato válido y su antigüedad, sin confundirlo con un fix actual.
4. Utilizar nuestro arbitraje de alimentación compartida con OLED y sensores, y equilibrar las referencias al rail.
5. Mantener despierto el procesador durante una adquisición que lo necesite; no tomar la bandera lógica del GPS como única prueba de su estado eléctrico.
6. Guardar las nuevas opciones mediante nuestros serializadores y archivos protegidos, con valores predeterminados que no cambien la configuración existente.
7. Revisar expresamente los cambios de `powersaving on/off` mientras el GPS está dormido, el primer arranque sin señal, la telemetría concurrente y los reinicios.

No recomiendo copiar íntegramente `LocationProvider` o `MicroNMEALocationProvider`: se perderían protecciones de nuestro arranque y control de UART, y se incorporaría una política de sincronización distinta.

Fuentes: [estado y temporizadores](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/src/helpers/sensors/LocationProvider.h#L12), [gestor GPS](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/src/helpers/sensors/EnvironmentSensorManager.cpp#L930), [cambio del flag](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/examples/companion_radio/MyMesh.cpp#L2213), [petición de telemetría](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/examples/companion_radio/MyMesh.cpp#L1734), [validez con corte en 2030](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/src/helpers/sensors/MicroNMEALocationProvider.h#L180), [último ajuste de límites](https://github.com/IoTThinks/MeshCore/commit/17e062ff6a1518740aebc120101ae024513a8832).

### 4. Ahorro mientras Bluetooth sigue activo: investigación separada

Es la aportación con mayor interés potencial para el uso con el teléfono conectado. Nuestro ahorro profundo de actividad sucede cuando el controlador BLE ya se ha detenido; el framework estable no tiene habilitada la gestión dinámica necesaria para reducir más el consumo con BLE activo.

PowerSaving 18 cambia a un paquete Arduino 2.0.17 personalizado y solicita 80/10 MHz, con `light_sleep_enable=false`, en su código actual. Por tanto, no aporta por ese camino el mismo sueño real de nuestro BLE apagado. Llama además a `esp_bt_sleep_enable()`; hay que comprobar tanto ese resultado como el de `esp_pm_configure()`, no dar por hecho el ahorro.

El repositorio del paquete personalizado contiene un README, pero no permite auditar allí una receta completa de construcción del framework. Se comprobó el metadato del archivo publicado: 434.076.334 bytes y SHA-256 `48b6ef1fd5b45d4560ea35672dd7e7f10a1c7157d30e70a63dcfba1c984e0936`. No se descargó ni instaló ese paquete durante esta auditoría; sus bibliotecas precompiladas no quedaron auditadas.

Recomendación: obtener configuración y construcción reproducible del framework, fijar su hash y preparar una prueba aislada para BLE activo. Mantener RX continuo, mapeo de TX, GATT estable, emparejamientos y el gestor actual de apagado BLE. Bloquear las reducciones de reloj o reposo incompatibles mientras USB/OTA o periféricos lo requieran. Medir corriente real antes de atribuirle un beneficio en nuestra placa.

Fuentes: [cambio del paquete](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/platformio.ini#L65), [implementación PM](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/src/helpers/PowerSaving.cpp#L14), [paquete publicado](https://github.com/IoTThinks/powersaving-arduino-lib/releases/tag/2.0.17), [nuestra infraestructura condicional](https://github.com/Amoulier/MeshCore/blob/62e184a6f104a0f75bb5054e64471275c553b6e8/variants/heltec_v4/HeltecV4Board.cpp#L222).

## Cambios que no deben sustituir nuestras mejoras

### RXPS y FEM

Dormir el ESP32 con el SX1262 escuchando continuamente y dormir la propia recepción del SX1262 son dos políticas diferentes. RXPS introduce intervalos en los que la radio no escucha; su recepción depende del preámbulo, SF, ancho de banda, transiciones y margen temporal. Las pruebas de cálculo no demuestran sensibilidad o entrega física equivalente.

La rama revisada activa RXPS nivel 6 y preámbulo 16 por defecto en los constructores de Companion, Repeater, Room Server y Sensor. La documentación de la propia rama identifica niveles 9 y 10 fuera de la regla temporal del fabricante; sus pruebas incluyen un caso descrito como pérdida de entrega en el extremo. La wiki todavía describe algunos valores antiguos. No recomiendo activar RXPS en nuestros perfiles estables ni prometer ausencia de pérdidas.

También cambia el LNA predeterminado hacia apagado si no hay una definición que lo habilite. Ya tenemos controles FEM y perfiles RX; adoptar ese valor puede reducir sensibilidad y alcance. Conservar `range` y el LNA donde el usuario los eligió.

La corrección de temporizador SX1262 de `Dev-PS18` sólo se usa al armar RXPS. Se basa en un modelo medido de un comportamiento no documentado del temporizador y añade cobertura de pruebas; no es una corrección pendiente para nuestro RX continuo. Si se ensaya RXPS en el futuro, deben revisarse conjuntamente ese ajuste y el resto del subsistema, manteniendo desactivado el modo por defecto.

El test estable `DefaultsKeepRepeaterDisabledWithBalancedIntent` verifica el valor inicial del struct RXPS, no los constructores de los firmwares que después lo habilitan. Por eso sus resultados no permiten concluir que el firmware tenga RXPS apagado por defecto. Los periodos manuales admitidos tampoco equivalen a configuraciones garantizadas de recepción: comprobar sólo su rango numérico no asegura que encajen en el preámbulo.

Fuentes: [predeterminados Companion](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/examples/companion_radio/MyMesh.cpp#L951), [RXPS de la rama](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/docs/cli_commands.md#L1217), [temporizador experimental](https://github.com/IoTThinks/MeshCore/commit/22c49c85119bcd74e02c409ec475f722cd65daf3), [wiki](https://github.com/IoTThinks/EasySkyMesh/wiki/PowerSaving).

### Potencia TX y retune

El cambio externo `743bb28f` fija 17 dBm como valor inicial del SX1262. Eso no equivale al bloqueo de máximo físico que ya aplicamos por tipo de FEM. Nuestro BLE y Solar conservan su objetivo nominal de 28 dBm; los demás roles conservan su techo actual estimado de 22 dBm. Son estimaciones de tablas, no mediciones RF.

No sustituir `mapRadioTxPower`, las tablas GC1109/KCT8103L, el margen de establecimiento del FEM o el bloqueo frente a comandos de menor potencia. Tampoco sustituir nuestro `idle()` obligatorio antes del retune: la preparación externa se centra en detener RXPS y no reproduce necesariamente nuestra corrección en recepción continua.

Fuentes: [potencia inicial externa](https://github.com/IoTThinks/MeshCore/commit/743bb28fc779713df2c5a43f0038d5bdc2b590bf), [mapeo propio](https://github.com/Amoulier/MeshCore/blob/62e184a6f104a0f75bb5054e64471275c553b6e8/variants/heltec_v4/HeltecV4Board.cpp#L556), [retune validado](HELTEC_V4_BLE_RETUNE_FIX_VALIDATION.md).

### Sueño, USB y Bluetooth

El bucle externo de Repeater usa pausas de hasta 30 segundos después de su espera inicial; Room Server usa 5 segundos y Sensor 30. No copiar esos bucles a Solar: eliminarían las comprobaciones específicas de USB, pantalla, GPS, estado RX y mantenimiento cada 20 ms que ya validamos. El soporte externo para despertar por botón tampoco sustituye nuestra restauración persistente de OLED/PRG.

Conservar el apagado asíncrono completo de BLE, los handles GATT estables después de reconstruirlo, la reconexión con el vínculo existente, los intervalos de conexión y anuncios adaptativos, y la protección mientras una app está conectada. Conservar asimismo la terminación de respuestas USB de 64 bytes y su espera acotada de escritura.

### Almacenamiento, validación y telemetría

| Área | Qué se perdería al reemplazarla con el árbol externo |
| --- | --- |
| Preferencias de Solar | Separación de `/repeater_prefs.json` y el perfil Companion `/prefs.json` |
| Persistencia | Archivos temporales, recuperación/backup, renombrado y guardas de escritura durante batería crítica |
| Identidad, contactos, ACL y regiones | Nuestras protecciones frente a una escritura incompleta |
| Repeater ANON | Helper que valida la longitud real de rutas antes de copiarlas |
| Companion | Comprobaciones adicionales de longitud y límites en frames/respuestas |
| Telemetría LPP | Correcciones de tamaño/escala de humedad, signos y validación de longitud antes de leer |
| Wi-Fi | Consulta de contraseña que informa si está configurada sin devolver su contenido |
| Energía y pantalla | ADC calibrado/promediado y caché; recuperación de batería crítica; VEXT, OLED y GPS coordinados |
| Calibración de ruido | Nuestra cadencia rápida inicial y mantenimiento de 30 segundos; el externo vuelve a 2 segundos de forma continua |
| Compilación/publicación | Diez roles, dependencias por función, SHA en firmware y nombres, verificación de imágenes y hashes |

Son diferencias constatadas del código; no se atribuyen todas a cambios recientes de EasySkyMesh. Muchas son correcciones que añadimos sobre su base común. Precisamente por eso un archivo que Git fusione sin conflicto puede reintroducir una conducta anterior.

En GPS/OLED hay diferencias eléctricas concretas: el externo no conecta ambos proveedores a nuestro gestor compartido de VEXT y declara otra polaridad para ese rail. También carece de nuestra detención de UART y del claim idempotente del GPS. Deben mantenerse nuestras conexiones y propietarios del rail; el pin VEXT de accesorios no debe confundirse con la alimentación del FEM.

Fuentes: [perfil y auditoría Solar](HELTEC_V4_SOLAR_REPEATER_AUDIT.md), [serialización externa](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/src/helpers/CommonCLI.cpp), [DataStore externo](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/examples/companion_radio/DataStore.cpp), [nuestro helper ANON](../src/helpers/RepeaterAnonRequest.h), [nuestro LPP](../src/helpers/sensors/LPPDataHelpers.h).

## Novedades experimentales: interés y problemas concretos

### Rutas de respuesta configurables y observación tras login

`Dev-PS18` permite seleccionar ruta directa/FLOOD y observar la ruta usada tras un login. Puede ayudar cuando el retorno es asimétrico o queremos diagnosticar una ruta. No es una mejora necesaria del ahorro de batería y no forma parte de `PowerSaving-v18` en el commit fijado.

Hallazgo estático que bloquea una copia literal:

1. La nueva política FLOOD se codifica como `out_path_len=0xFE`.
2. Un login DIRECT no reinicia ese valor; el restablecimiento se hace para login FLOOD.
3. `onAnonDataRecv()` considera que hay ruta con cualquier valor distinto de `0xFF`, por lo que trata `0xFE` como ruta directa.
4. La respuesta se envía con ese valor reservado y la validación de ruta del paquete lo rechaza. El login puede quedarse sin respuesta.
5. Ese valor se puede guardar en contactos; Room Server y un firmware anterior tienen consumidores que sólo distinguen `0xFF`, creando otra incompatibilidad al cambiar de rol o volver de versión.

Recomendación: posponer. Si se adopta, separar la política de envío del campo de longitud codificada o auditar todos sus consumidores y la migración. Añadir pruebas integradas de login, respuesta, persistencia, downgrade y hash de 1/2/3 bytes. Las siete pruebas del helper de observación pasan, pero no ejercitan ese recorrido de login.

Fuentes: [sentinelas de ruta](https://github.com/IoTThinks/MeshCore/blob/b568b526fb00be4b802beb3cbf45d580fc480577/src/helpers/ClientACL.h#L13), [login](https://github.com/IoTThinks/MeshCore/blob/b568b526fb00be4b802beb3cbf45d580fc480577/examples/simple_repeater/MyMesh.cpp#L135), [decisión y envío](https://github.com/IoTThinks/MeshCore/blob/b568b526fb00be4b802beb3cbf45d580fc480577/examples/simple_repeater/MyMesh.cpp#L609), [helper probado](https://github.com/IoTThinks/MeshCore/blob/b568b526fb00be4b802beb3cbf45d580fc480577/src/helpers/ClientPathObservation.h).

### Medidor INA228

Es una opción interesante para medir corriente, potencia y energía reales en una instalación solar, si se incorpora el componente físico y se calibra su shunt. El soporte nuevo está condicionado por una opción de compilación; añadir sólo el código no mide el consumo de nuestro Heltec actual.

Se encontró un error de unidades: `query_ina228()` pasa `getWattHour()` a `addEnergy()`, cuya unidad es kWh. El valor publicado queda multiplicado por 1.000 respecto a esa unidad. Adaptar con la conversión correcta y probar un caso conocido: 1.000 Wh deben publicarse como 1 kWh. Conservar las dependencias por rol y la gestión del rail de sensores.

Fuentes: [adición](https://github.com/IoTThinks/MeshCore/commit/9ea68bdc), [llamada con unidades incompatibles](https://github.com/IoTThinks/MeshCore/blob/b568b526fb00be4b802beb3cbf45d580fc480577/src/helpers/sensors/EnvironmentSensorManager.cpp#L452), [CayenneLPP API](https://github.com/ElectronicCats/CayenneLPP/blob/master/API.md).

## Funciones estables de prioridad menor o ya cubiertas

### Reinicio periódico y conservación de hora

El reinicio programado puede servir como medida operativa opcional, pero interrumpe servicio y necesita drenar transmisiones y guardar cambios. La CLI externa lo anuncia también para Room Server, aunque no se encontró allí el mismo bucle de reinicio de Repeater/Sensor. No recomiendo incorporarlo como remedio general de una radio que deja de recibir.

La conservación adicional de hora introducida para nRF52 no aporta una función nueva a Heltec. Nuestro ESP32-S3 ya conserva un respaldo validado de hora en RAM RTC y lo actualiza sin escrituras continuas a flash, igual que la base externa. Eso no garantiza conservar una hora válida tras una pérdida total de alimentación. Mantener esa implementación y la detección de RTC externo por rol. Fuente: [nuestro ESP32RTCClock](https://github.com/Amoulier/MeshCore/blob/62e184a6f104a0f75bb5054e64471275c553b6e8/src/helpers/ESP32Board.h#L220).

### Diagnóstico CAD y TCXO

El contador externo de CAD ocupado puede adaptarse sin cambiar la radio ni activar RXPS: sirve para observar contención, no para atribuir por sí solo una pérdida de mensajes. La reducción global del tiempo TCXO a 1.600 µs se fundamenta en mediciones publicadas de otros módulos; Heltec V4 no aparece entre los módulos citados en ese comentario. Con nuestro RX continuo no existe el mismo ahorro repetido por cada despertar RXPS. Mantener el tiempo actual hasta medir nuestra placa, especialmente en frío y durante transiciones. Fuentes: [contador CAD](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/src/helpers/radiolib/RadioLibWrappers.cpp#L480), [valor TCXO global](https://github.com/IoTThinks/MeshCore/blob/17e062ff6a1518740aebc120101ae024513a8832/platformio.ini#L31).

## Hallazgo adicional compartido: longitud de PATH

En ambos árboles, `Mesh::onRecvPacket()` valida el encoding de una ruta PATH descifrada, pero avanza hasta `extra_type` sin comprobar primero que la longitud declarada cabe en el plaintext recibido. En un PATH autenticado truncado puede leer datos fuera de la parte válida y la resta usada para `extra_len` puede producir un valor no representativo del contenido disponible. Es una debilidad compartida anterior a las novedades experimentales, distinta del guard ANON que ya añadimos. No se ha demostrado una explotación ni se enviaron paquetes de prueba a los nodos.

Recomiendo priorizar una comprobación de longitud disponible en ese punto y pruebas de truncamiento antes de ampliar la observación o selección de rutas: `1 + path_bytes + 1 <= len`, contando el byte inicial, la ruta y `extra_type`, antes de leer este último. El helper experimental comprueba encoding y capacidad de destino, pero no sustituye ese control del origen. Fuente propia: [parser PATH](https://github.com/Amoulier/MeshCore/blob/62e184a6f104a0f75bb5054e64471275c553b6e8/src/Mesh.cpp#L160).

## Pruebas ejecutadas y límites de la evidencia

| Comprobación realizada para esta revisión | Resultado |
| --- | --- |
| SHA de nuestro `main` local frente a `origin/main` | Ambos `62e184a6`; árbol de firmware sin modificaciones |
| Referencias remotas y documentación de PowerSaving18 | Fuente real y ramas identificadas y fijadas |
| Diferencias respecto a base oficial y respecto a nuestro fork | Inventario y lista de commits guardados |
| Simulación de fusión en copia aislada | 152 rutas en conflicto; no se aplicó |
| Suite nativa RXPS de PowerSaving-v18 | 45/45 aprobadas, 13,605 s |
| Suites nativas de Dev-PS18: RXPS, temporizador SX1262 y observación de ruta | 60/60 aprobadas, 8,955 s; 45 RXPS/CLI, 8 temporizador y 7 observación |
| Instalación/compilación de firmware externo para un nodo | No realizada en esta fase de análisis |
| Corriente, autonomía, potencia RF, sensibilidad o pérdida física con cambios nuevos | No medidas en esta revisión |

Las 90 pruebas de nuestro código y la matriz de diez variantes corresponden a la preparación de v1.1.2 ya publicada; no se presentan como pruebas nuevas de una integración con EasySkyMesh. Las suites ejecutadas ahora prueban lógica de ordenador, no el driver completo sobre radio, el stack BLE ni las rutas de login integradas. Los hallazgos de rutas/INA228 son de revisión de código, no fallos reproducidos en los nodos.

Los valores de autonomía y corriente publicados por el proyecto externo dependen de placa, radio, GPS, LNA y perfil. No se trasladan a nuestro Heltec V4.3 de alta potencia ni se convierten en una promesa de autonomía.

## Orden de integración propuesto

1. **Corregir el hallazgo compartido de PATH y adaptar cambios pequeños:** validar el tamaño real de PATH, completar cola entrante/trabajo pendiente, diagnóstico de pines y reinicio, estado/uptime en OLED y, opcionalmente, contador CAD. Un cambio por tema, con pruebas pertinentes y sin alterar las políticas de radio/energía.
2. **GPS opcional:** planificador con estados y temporizadores probados, sin modificar el comportamiento de `gps off` ni las preferencias existentes.
3. **Ensayo de framework para BLE activo:** construcción reproducible, control de errores, comparación de consumo y validación completa de USB/BLE/LoRa antes de proponerlo como estable.
4. **Sólo si hay necesidad concreta:** INA228 corregido y hardware disponible; rutas avanzadas después de resolver el caso FLOOD y la compatibilidad de contactos. RXPS queda separado y desactivado por defecto.

No hace falta añadir otra variante de bajo consumo: las mejoras compatibles deben seguir dentro del BLE y Solar que conservamos. Tampoco se reintroducen placas, TFT/R8 o variantes retiradas como efecto lateral de portar una función.

### Condiciones para aceptar cualquier integración

| Área que debemos preservar | Verificación necesaria |
| --- | --- |
| TX máximo por FEM | Revisar valor pedido/aplicado/estimado; comprobar que una preferencia o comando inferior no reduce el objetivo actual |
| RX continuo y retune | Confirmar RX sin duty cycle, cambios de SF/BW/frecuencia y retorno sin reinicio; tráfico conocido y contadores |
| BLE | Reconexión con vínculo existente, ciclos de apagado/restauración, handles estables y ausencia de cortes mientras está conectado |
| USB | Diagnóstico continuo, frames fragmentados, respuestas de 64 bytes y protección de sueño con host conectado |
| PRG/OLED/VEXT | Restauración persistente, ventana de visibilidad, apagado posterior y sensores/GPS que comparten alimentación |
| GPS | Arranque desactivado sin sondeo/UART; adquisición correcta, timeout, cambios de ahorro y telemetría con GPS apagado |
| Batería y almacenamiento | Lecturas de seguridad sin caché, recuperación crítica, guardas de escritura, archivos por rol y recuperación tras una escritura interrumpida |
| Protocolo y datos | Bounds ANON/frames, LPP, suscripciones, contactos/ACL y ajustes sin filtración de credenciales |
| Distribución | Pruebas nativas pertinentes, diez variantes compiladas, hashes y ausencia de hooks de diagnóstico en imágenes normales |

Una compilación correcta no basta para afirmar compatibilidad en todas estas áreas. Las pruebas físicas requerirán una fase de integración posterior y no se sustituyen por las suites nativas de esta revisión.

## Evidencia local

En `out/easyskymesh-audit-2026-11-09/` quedan las copias fijadas de ambos proyectos y de la rama experimental, el inventario de commits/archivos, la salida de la simulación y los registros de pruebas:

- `comparison-manifest.json`
- `powersaving-commits.txt`
- `powersaving-v18-changed-files.tsv`
- `our-fork-vs-powersaving-files.tsv`
- `merge-tree-dry-run.txt`
- `rxps-native-tests.log`
- `dev-rxps-path-tests.log`

Las copias y registros están dentro de `out/`, ignorado por Git. Se conserva el aviso MIT del código revisado; cualquier adaptación futura debe conservar los avisos de licencia y atribución que correspondan.
