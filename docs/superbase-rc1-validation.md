# Muzi Works SuperBase: revisión de alcance de RC1

Fecha de revisión: **27 de octubre de 2026**, America/La_Paz.
Versión candidata: **v1.18-sb2-rc1**. Hardware: **Base Uno nRF52840/SX1262 +
Super IO**, Companion MeshCore para móvil por Bluetooth.

## Qué significa esta candidata

El alcance funcional se congela con las mejoras ya acordadas. No se añaden
funciones. La candidata usa el perfil de producción optimizado, sin los comandos
USB de diagnóstico ni las opciones de compilación debug. Los binarios de
desarrollo, registros internos y datos de dispositivos no se publican.

**La RC1 exacta no se ha instalado ni ensayado físicamente.** Las pruebas físicas
descritas abajo corresponden a imágenes internas anteriores de la misma
implementación. La compilación optimizada y la auditoría del paquete comprueban
esta candidata; no convierten automáticamente esas pruebas anteriores en una
certificación del binario nuevo. Por ello se publica como **prerelease**.

## Matriz de implementación y verificación

| Mejora acordada | Implementación | Verificación disponible y límite |
| --- | --- | --- |
| BAS de batería Bluetooth | Porcentaje 0–100, reintentos, actualización al reconectar y suscribirse | Regresiones de errores y suscripción tardía. Lectura y notificación comprobadas en tres reconexiones Windows; porcentaje mostrado por MeshCore Android. No es una estimación calibrada de autonomía. |
| Cargador BQ25185 | STAT1 con pull-up; estados de carga/reposo/fallo; sin aviso de fallo a batería | El usuario confirmó desaparición de `! Check charger`. Entradas de drenador abierto y fallos simulados cubiertos por regresiones. No se han provocado fallos eléctricos reales ni medido corriente de carga. |
| Bluetooth desde la pantalla | Habilitación en ejecución con preferencia persistente | Código revisado y conexión Android comprobada. Falta una secuencia física completa de apagar/encender desde la página y reiniciar después en RC1. |
| Buzzer y políticas de sonido | All, Off, Messages, System, DMs Only; persistencia y parada al silenciar | Audio audible confirmado. Políticas, copia de melodías y propiedad entre instancias cubiertas por regresiones. Falta recorrer físicamente todos los modos y su persistencia en RC1. |
| Navegación con seis controles | Captura de flancos, rebotes, pulsación larga, triple Atrás y alternativa por sondeo | El usuario confirmó respuesta de los seis controles. Regresiones de flancos, rebotes, desbordamiento y sondeo. La captura por interrupciones no está aprobada en el hardware; véase el límite de uso más abajo. |
| GPS | Mode 2 y preferencia de software; evita arranques duplicados y descarta fix anterior al reiniciar el receptor | Posición real a cielo abierto; 28 muestras durante 309,6 s a batería con 16–19 satélites y sin reinicios observados. No garantiza recepción interior, arranque en cualquier ubicación ni estabilidad prolongada. |
| Energía y apagado | Reposo solicitado al IMU/magnetómetro; apagado de buzzer, pantalla y GPS | Registros de reposo leídos en hardware. Arranque a batería observado. No hay medición instrumental de consumo ni descarga completa. |
| Preferencias y preparación de apagado | Archivo temporal y sustitución por rename; reintento; vaciado de caché y contactos pendientes | Regresiones de apertura, escritura incompleta, borrado y rename fallidos. No es una transacción conjunta de preferencias y contactos; el guardado de contactos heredado no informa todos sus fallos. No se ha ensayado un corte de alimentación durante escritura. |
| Radio SX1262 | Reintentos espaciados, reinicialización tras fallos RX y restauración de parámetros; protección de TX activa | Anuncio y mensaje directo entrante confirmado con Heltec V4 después de intercambiar anuncios. Recuperación RX y protección TX probadas con fallos simulados. No hay prueba de alcance, respuesta escrita desde SuperBase ni recuperación de un fallo físico inducido. |
| Actualización Bluetooth | Guardado previo, rechazo ante fallo/timeout; corrección de USB, RAM temporal del cargador y tabla GATT | Una reinstalación completa con Nordic Android en valores predeterminados: validación y arranque automático, sin RESET ni reconectar USB; después reconexión MeshCore. Es una prueba de la implementación interna, no una prueba OTA del binario RC1. |
| Emojis | UTF-8 y 3.953 secuencias OpenMoji 17.0, recorte y salto de línea sin partir secuencias | Catálogo completo verificado por pruebas; límites de memoria con sanitizadores. El usuario confirmó el joystick en el nombre real. Monocromo a 10 × 10 píxeles: colores y algunos detalles se pierden. |
| RTC | Se conserva el reloj heredado de MeshCore; no se añade una función nueva | Lectura y avance de segundos del RX8130CE comprobados en Wire1. No se han medido precisión, retención ni integración completa como fuente de hora. |

## Fallos observados y efecto real en el uso

- **Aviso falso del cargador: corregido.** La entrada STAT1 flotante causaba el
  aviso; el pull-up conserva la detección de estados y se confirmó la desaparición.
- **Bloqueo al terminar DFU: corregido en la implementación ensayada.** Se
  apagó USB antes del salto al cargador y se reservó su RAM temporal. El ajuste
  del registro de vínculo solo se aplica al cargador cuya firma se reconoce;
  no se escribe ni actualiza el bootloader. La prueba con Nordic predeterminado
  pasó un ciclo completo. Falta repetir con esta RC1 exacta.
- **Bloques en lugar del emoji: corregido y observado en pantalla.** Se mantiene
  el nombre UTF-8 original; el selector de presentación ya no genera otro bloque.
- **Captura de botones por interrupciones: pendiente.** El monitor informó
  máscara y contadores a cero, aunque los seis controles respondieron. El sondeo
  alternativo está implementado y probado. No se puede prometer que se conserve
  una pulsación muy breve ocurrida íntegramente entre dos consultas o durante una
  operación bloqueante. No se ha reproducido un bloqueo de navegación normal.
- **Mensaje directo sin confirmación inicial: resuelto al intercambiar anuncios.**
  El remitente pasó a ser conocido y el usuario confirmó la entrega posterior.
  Ese caso no demuestra un defecto persistente de radio.
- **GPS sin fix dentro de la casa: límite de recepción observado.** Con cielo
  abierto obtuvo posición. No se atribuye este caso a un fallo del firmware.
- **Pantalla negra por inactividad:** el apagado por temporizador es esperado;
  no equivale a un fallo de arranque. Los bloqueos históricos posteriores a DFU
  se distinguen por falta de respuesta y se trataron por separado.

## Qué falta antes de promoverla a estable

1. Instalar el binario RC1 exacto y comprobar arranque, pantalla, controles,
   Bluetooth, batería y conservación de preferencias.
2. Completar un ciclo OTA de RC1 con Nordic en sus valores predeterminados y
   reconectar MeshCore, sin cambiar ajustes del teléfono.
3. Probar envío y recepción desde el móvil por ambos nodos y alternar GPS y
   Bluetooth desde sus controles, incluido su estado después de reiniciar.
4. Repetir uso continuado a batería y una descarga completa; medir consumo si
   se desea aprobar una mejora cuantitativa de autonomía.
5. Revisar las interrupciones de botones antes de prometer captura de pulsaciones
   durante esperas. Los límites de alcance RF, fallos eléctricos y cortes de
   alimentación se mantienen explícitos hasta ensayarlos.

No forman parte del alcance: ACK por MQTT, cambios de seguridad específicos de
Meshtastic, teclado autónomo, brújula o temporizador de posición estacionaria de
12 horas. No se han cambiado el protocolo MeshCore, la configuración de radio
del usuario ni los datos de identidad para preparar esta candidata.
