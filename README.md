# freertos-task-monitor

STM32F407 Discovery üzerinde FreeRTOS ile çalışan 4 task ve UART üzerinden komut kabulu. Tek ekstra donanım USB-to-UART adaptör.

## Sistem

- **ADC Task** — dahili sıcaklık sensörü + Vref, 1 sn aralıklı
- **LED Task** — queue'dan gelen komuta göre farklı blink pattern
- **Shell Task** — UART CLI, 115200 baud
- **Monitor Task** — 10 sn'de bir task listesi + CPU%, heap bilgisi basar

Heartbeat için software timer kullanıldı, yeşil LED (PD12) 1 Hz.

## Bağlantı

```
PA2 → TX (adaptör RX)
PA3 → RX (adaptör TX)
GND → GND
```

## Komutlar

```
> tasks
> stats
> adc
> led idle | busy | error | off
> uptime
> help
```

## CubeIDE kurulum

1. STM32F407VGTx projesi oluştur, FreeRTOS kernel source ekle
2. `Core/Src` ve `Core/Inc` dosyalarını kopyala
3. Üretilen `FreeRTOSConfig.h` yerine repodakini koy
4. Build + flash, seri terminal 115200 8N1

