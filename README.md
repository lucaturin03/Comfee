# Comfee

Firmware per ESP32 che collega un climatizzatore **Comfee** (o un altro modello basato sul protocollo **Midea**) ad **Apple HomeKit**, senza bridge, senza cloud e senza Home Assistant.

L'ESP32 parla con il climatizzatore via UART usando la libreria [MideaUART](https://github.com/dudanov/MideaUART) e si presenta a HomeKit come accessorio nativo grazie a [HomeSpan](https://github.com/HomeSpan/HomeSpan). Il climatizzatore compare nell'app Casa come "Climatizzatore" e si può controllare da iPhone, iPad, Apple Watch, Mac e con Siri.

## Funzionalità

- Accensione e spegnimento
- Modalità **Raffreddamento**, **Riscaldamento** e **Automatico**
- Impostazione della temperatura obiettivo (17–30 °C, passo 1 °C)
- Lettura della temperatura interna rilevata dal climatizzatore
- Stato attuale (spento / inattivo / riscalda / raffredda) sincronizzato in tempo reale
- Le modifiche fatte dal telecomando originale vengono riportate in HomeKit

### Mappatura HomeKit ↔ climatizzatore

| HomeKit (Heater Cooler) | Climatizzatore Midea |
|---|---|
| Attivo / Inattivo | Power on / off |
| Raffredda | `MODE_COOL`, setpoint = soglia di raffreddamento |
| Riscalda | `MODE_HEAT`, setpoint = soglia di riscaldamento |
| Automatico | `MODE_AUTO`, setpoint = media tra le due soglie |

La ventola viene sempre impostata su `FAN_AUTO`. Le modalità Deumidificazione e Solo ventola non sono esposte (HomeKit non le prevede nel servizio Heater Cooler); se attivate dal telecomando, HomeKit mostra lo stato "inattivo".

Lo stato attuale viene calcolato confrontando la temperatura ambiente con il setpoint (isteresi di 0,3 °C in Riscalda/Raffredda e 0,5 °C in Automatico).

## Hardware

- Una scheda ESP32 (il progetto è configurato per `esp32dev`, vedi [Configurazione](#configurazione))
- Un collegamento UART alla porta del climatizzatore pensata per il modulo Wi-Fi/USB (connettore del dongle Midea)
- Un convertitore di livello logico 5 V ↔ 3,3 V, perché la UART del climatizzatore lavora tipicamente a 5 V mentre l'ESP32 accetta solo 3,3 V

### Collegamenti

| ESP32 | Climatizzatore |
|---|---|
| GPIO 4 (RX) | TX |
| GPIO 5 (TX) | RX |
| GND | GND |

La comunicazione avviene a 9600 baud, 8N1. I pin si cambiano in `src/main.cpp`:

```cpp
#define PIN_UART_RX 4
#define PIN_UART_TX 5
```
