# Muzi Works SuperBase: revisión de alcance de RC1

Revisión de alcance iniciada: 27 de octubre de 2026, America/La_Paz.
Versión: **v1.18-sb2-rc1**. Hardware: **Base Uno nRF52840/SX1262 + Super IO**.
Uso: Companion MeshCore por Bluetooth.

## Estado de la candidata

**Publicación detenida. No hay una candidata aprobada como estable.**
El alcance permanece en las mejoras acordadas. No se añaden funciones. El perfil
normal usa optimización de producción, sin comandos USB de diagnóstico ni trazas.
Los binarios internos, datos del nodo y registros de desarrollo no se publican.

Los resultados físicos siguientes no se transfieren automáticamente a cada
recompilación. Se identifica qué implementación se ensayó y qué falta repetir
sobre el instalable final. Compilación y auditoría no certifican el hardware.

El instalable de aplicación que se está validando ahora es el ZIP de producción
con SHA-256 `ec7f774c2ac15d2d79518b49a2b2ee51bf0ad09b35e47185528d8670f7bc7e50`,
compilado desde la revisión de código `12b00de0a8229c2d8c4d00c872eed159978e5fd9`.
Se instaló inicialmente por USB y se comprobaron consultas nuevas de versión e
identidad: `v1.18-sb2-rc1` y `Muzi Works SuperBase`. Después se completó una
reinstalación OTA de este mismo archivo con Nordic en valores predeterminados.
USB regresó automáticamente en 49,44 s desde su desaparición y MeshCore volvió
a conectar conservando el vínculo anterior, sin PIN nuevo, RESET, reconexión
del cable ni cambios de ajustes del teléfono. **Ese ciclo OTA final pasó.**
Las pruebas físicas de otras imágenes RC1 no se presentan como pruebas finales
de este archivo. Este resultado corresponde a un ciclo con USB conectado;
no valida autonomía ni equivale a aprobar las demás funciones pendientes.

GitHub Actions no ejecutó el flujo nuevo porque aún no existe en la rama
predeterminada. Las compilaciones ARM, regresiones Windows y sanitizadores
Linux/WSL se realizaron localmente. No se atribuye aprobación de CI.
Las correcciones de suscripciones Bluetooth descritas abajo pasaron las
regresiones nativas, AddressSanitizer/UndefinedBehaviorSanitizer y la compilación
ARM. La comparación del vínculo antes y después del ciclo OTA final confirmó
el mismo evento de emparejamiento y conexión MeshCore sin una nueva solicitud.

## Implementación y verificación

| Mejora acordada | Implementación y evidencia | Validación pendiente |
| --- | --- | --- |
| Batería Bluetooth | BAS 0–100, reintentos y nueva suscripción. Regresiones de entrega; lectura/notificación Windows en imágenes internas y porcentaje Android en RC1. | Repetición sobre instalable final y calibración si se promete autonomía. |
| Cargador BQ25185 | STAT1 con pull-up y distinción carga/reposo/fallo. Usuario confirmó desaparición de `! Check charger`; entradas y fallos simulados cubiertos. | Fallos eléctricos inducidos y corriente de carga sin medir. |
| Bluetooth desde pantalla | Habilitación en ejecución y preferencia persistente; conexión y reconexión Android observadas en RC1. | Apagar/encender desde sus controles y reiniciar, comprobando persistencia. |
| Buzzer | Modos All, Off, Messages, System y DMs Only, persistencia y parada al silenciar; regresiones de políticas y propiedad. Aviso real por DM recibido en RC1 confirmado audible. | Recorrer físicamente todos los modos y reiniciar sobre el instalable final. |
| Seis controles | Captura de flancos, rebotes, pulsación larga, triple Atrás y sondeo alternativo; regresiones. Usuario confirmó pantalla y seis controles en RC1. Captura real previa: máscara 63, dos flancos adicionales en cada uno de los seis controles, cero descartes. | Repetir sobre el instalable final; no se afirma captura infalible durante cualquier operación bloqueante. |
| GPS | Mode 2 y preferencia de software, control de arranque y descarte de posición antigua. Imagen interna: 28 muestras durante 309,6 s a batería, 16–19 satélites y sin reinicios observados. | Nueva adquisición exterior y alternancia física en el instalable final. No se promete recepción interior. |
| Energía y apagado | Reposo IMU/magnetómetro, parada de buzzer/pantalla/GPS y arranque a batería observados en imágenes internas. | Sesión prolongada, descarga completa y consumo instrumental. |
| Preferencias | Temporal + rename, reintento y vaciado previo; pruebas de apertura, escritura incompleta, borrado y rename fallidos. | Persistencia física final y corte de alimentación durante escritura. Contactos heredados no informan todos sus errores; no hay transacción conjunta. |
| Radio SX1262 | Recuperación RX y protección TX con fallos simulados. RC1: mensajes directos en ambos sentidos con Heltec V4, confirmados como entregados y observados en cada receptor. | Repetición sobre instalable final; alcance y fallo físico inducido sin ensayar. |
| Actualización Bluetooth | Guardado previo, corrección de USB y RAM de intercambio, autorización S140 y entrada cifrada con vínculo compartido. Ambos arreglos CCCD pasan pruebas nativas, sanitizadores y compilación ARM. **Ciclo final aprobado:** ZIP `ec7f774…` con cargador `v2.3-muzi.2-rc1`, Nordic predeterminado, transferencia y validación completas, USB automático en 49,44 s y reconexión MeshCore con el mismo vínculo, sin nuevo PIN, RESET ni reconectar USB. | Alcance de la evidencia: un ciclo con USB conectado y esta combinación exacta; no valida autonomía, otras versiones de cargador ni todos los teléfonos. Sigue pendiente el arranque normal a batería y la persistencia de funciones indicadas en las otras filas. |
| Emojis | UTF-8, 3.953 secuencias OpenMoji 17, recorte y saltos sin partir secuencias. Catálogo y límites con sanitizadores. Usuario confirmó joystick en nombre real y pantalla RC1. | Repetición visual final. Monocromo de 10 × 10: no conserva color ni todo el detalle. |
| RTC | Se conserva el reloj heredado. Lectura y avance de segundos RX8130CE observados en imagen interna. | Precisión y retención no medidas; no se añade otra integración de hora. |

## Fallos y efecto real

- **Aviso falso de cargador:** corrección observada y cubierta por regresiones.
- **Bloques del emoji:** corrección observada; se conserva el nombre UTF-8.
- **Radio sin confirmación inicial:** resuelto tras intercambiar anuncios; no
  se reprodujo como fallo persistente. RC1 tuvo comunicación bidireccional.
- **Captura por interrupciones:** los contadores a cero de una prueba anterior
  no describen la última evidencia. Los seis controles produjeron flancos reales
  antes de instalar RC1. Se mantiene el sondeo alternativo.
- **Pantalla negra:** el apagado por inactividad se distingue del bloqueo DFU.
  El usuario confirmó retorno automático y despertar con centro en la RC1 anterior.
- **Pérdida de emparejamiento tras OTA:** fue un fallo real del flujo y bloqueó
  su aprobación. Nordic Legacy elimina el vínculo con sus valores predeterminados.
  La entrada con vínculo compartido evita pedir ese borrado, pero exige que el
  cargador y la aplicación anuncien correctamente sus cambios de servicios.
  Un emparejamiento manual completado por el usuario después de un fallo anterior
  establece la base para el nuevo ensayo; no se cuenta como conservación del
  vínculo durante aquel fallo. **El ciclo OTA final sí conservó ese vínculo
  existente y reconectó MeshCore sin volver a emparejar.**
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
  se modifica. **La reconexión tras OTA del ZIP final pasó y mantuvo el mismo
  vínculo, sin solicitar otro PIN.**

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

**Un ciclo OTA final con esta combinación exacta de cargador y aplicación pasó.**
La firma de su rutina de intercambio coincide con la que reconoce MeshCore y se
comprobaron físicamente transferencia, validación, arranque automático y
reconexión con el vínculo conservado. El cargador compatible requerido para este
resultado es `v2.3-muzi.2-rc1`; no se extiende la aprobación al cargador anterior.
La futura distribución debe identificar esa dependencia y sus resultados de
validación. **La publicación continúa detenida por las demás validaciones
pendientes; estos archivos siguen sin publicar.**

## Validaciones antes de publicar

1. Completar el arranque normal a batería y la reconexión conservando el vínculo
   después de reiniciar, sin atribuir autonomía al ciclo OTA con USB ya aprobado.
2. Mantener identificados los instalables finales y completar pantalla, seis
   controles, batería Bluetooth y mensajes directos en ambos sentidos sobre ellos.
3. Recorrer controles de Bluetooth, GPS y sonido; comprobar preferencias después
   de reiniciar y nuevas posiciones exteriores con GPS encendido/apagado.
4. Completar la sesión a batería y revisar las mediciones/ensayos pendientes.
   No presentar autonomía, alcance o robustez eléctrica como aprobados sin evidencia.
5. Actualizar esta matriz con resultados del instalable exacto y verificar
   hashes, licencias, fuente y exclusión de diagnósticos antes de publicar.

No forman parte del alcance: ACK por MQTT, cambios de seguridad propios de
Meshtastic, teclado autónomo, brújula ni temporizador estacionario de 12 horas.
No se han exportado identidad, claves ni configuración del nodo. Se conservan el
protocolo MeshCore y los parámetros de radio del usuario. Los arreglos de
persistencia CCCD no borran ni recrean los campos de claves o nombre del vínculo.
