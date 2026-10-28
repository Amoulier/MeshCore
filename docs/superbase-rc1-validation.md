# Muzi Works SuperBase: revisión de alcance de RC1

Revisión: 27 de octubre de 2026, America/La_Paz.
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

GitHub Actions no ejecutó el flujo nuevo porque aún no existe en la rama
predeterminada. Las compilaciones ARM, regresiones Windows y sanitizadores
Linux/WSL se realizaron localmente. No se atribuye aprobación de CI.

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
| Actualización Bluetooth | Guardado previo, corrección de USB y RAM de intercambio, autorización S140, entrada cifrada con vínculo compartido. Un ciclo RC1 normal transfirió, validó y arrancó automáticamente; USB regresó en 48,49 s sin RESET ni reconectar. | **Bloqueante:** aquel ciclo eliminó el emparejamiento y exigió otro PIN. La entrada que conserva el vínculo aún falla por caché GATT en el cargador, antes de transferir datos. Requiere ciclo completo y reconexión sin emparejar de nuevo. |
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
- **Pérdida de emparejamiento tras OTA:** fallo real del flujo y bloqueo de
  aprobación. Nordic Legacy elimina el vínculo con sus valores predeterminados.
  La entrada con vínculo compartido evita pedir ese borrado, pero exige que el
  cargador anuncie correctamente el cambio de servicios.
- **Caché GATT al entrar al cargador:** reproducida en dos secuencias, sin
  transferencia de firmware. El último intento confirmó la orden antes del
  cambio de servicios y aun así terminó con `GATT INVALID HANDLE`. Recuperación
  por RESET confirmada por el usuario; no se cuenta como arranque automático.

El binario publicado del cargador v2.3-muzi.1 usa el inicio fijo 0x000C para
Service Changed y oculta el error de atributo inválido. S140 6.1.1 reserva los
atributos hasta 0x000D. Está preparada una corrección mínima que utiliza el
handle real asignado a su servicio DFU. Compilación y auditoría estática pasan;
**ese cargador corregido todavía no se ha instalado ni validado físicamente**.
No se modificó el cargador instalado durante estas pruebas.

## Validaciones antes de publicar

1. Resolver y probar OTA con conservación del emparejamiento, valores Nordic
   predeterminados, retorno automático y reconexión MeshCore sin PIN nuevo.
2. Fijar los instalables finales y repetir arranque, identidad, pantalla, seis
   controles, batería Bluetooth y mensajes directos en ambos sentidos.
3. Recorrer controles de Bluetooth, GPS y sonido; comprobar preferencias después
   de reiniciar y nuevas posiciones exteriores con GPS encendido/apagado.
4. Completar la sesión a batería y revisar las mediciones/ensayos pendientes.
   No presentar autonomía, alcance o robustez eléctrica como aprobados sin evidencia.
5. Actualizar esta matriz con resultados del instalable exacto y verificar
   hashes, licencias, fuente y exclusión de diagnósticos antes de publicar.

No forman parte del alcance: ACK por MQTT, cambios de seguridad propios de
Meshtastic, teclado autónomo, brújula ni temporizador estacionario de 12 horas.
No se han cambiado identidad, claves, canales ni ajustes de radio del usuario.
