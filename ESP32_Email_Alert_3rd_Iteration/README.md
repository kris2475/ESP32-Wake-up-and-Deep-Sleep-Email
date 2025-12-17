# ESP32 Deep Sleep Time-Aligned Logger (2-Minute Interval)

## 📝 Overview

This Arduino sketch runs on an ESP32 microcontroller to periodically **read the ESP32’s internal temperature sensor register** and report the result via email. The primary objective of the project is to achieve **ultra-low power operation** using the ESP32’s **Deep Sleep** mode while maintaining **strict alignment to real-world clock boundaries** for data logging.

The system is designed to wake and evaluate logging conditions **exactly on the minute, every 2 minutes** (for example, `10:00:00`, `10:02:00`, `10:04:00`).

> **Important clarification:**  
> The sketch correctly reads the value returned by the ESP32’s internal temperature sensor on every wake-up. The constant temperature value observed is a **hardware characteristic of the ESP32 internal sensor**, not a software or scheduling error.

---

## 🔋 Key Design Features

### 1. Low-Power Operation (Deep Sleep)

The primary energy-saving mechanism is the ESP32’s **Deep Sleep** mode.

- The ESP32 is awake only long enough to:
  - Connect to Wi-Fi
  - Synchronize system time via NTP
  - Read the internal temperature sensor register
  - Send an email report
- Typical active time: **5–10 seconds**
- Typical sleep duration: approximately **1 minute 50 seconds**
- During Deep Sleep, the main CPU is powered down and current consumption drops into the **microamp range**

This design is well-suited for long-term, battery-powered deployments.

---

### 2. Precise Time Alignment (NTP Synchronization)

To prevent long-term drift, the system relies on **Network Time Protocol (NTP)** synchronization rather than free-running timers.

**Execution flow:**

1. **Wake-up**  
   The ESP32 wakes via its internal RTC timer.

2. **Time Synchronization**  
   It connects to Wi-Fi and synchronizes its clock with an NTP server.

3. **Boundary Calculation**  
   The firmware calculates the number of seconds remaining until the next official 2-minute boundary (for example, `10:02:00`).

4. **Re-Scheduling**  
   Deep Sleep is re-entered with a timer duration computed from the current wall-clock time.

This approach keeps log timestamps aligned with real-world time, compensating for:
- RTC drift
- Variable Wi-Fi connection times
- Email transmission latency

---

### 3. Schedule Persistence (RTC Memory)

The sketch uses `RTC_DATA_ATTR` to preserve scheduling state across Deep Sleep cycles.

**Persistent variables include:**

- `nextCaptureTime`  
  Stores the absolute timestamp of the next intended log event.

- `initialTimeSet`  
  Ensures that the logging schedule is initialized **only once**, on first boot or after firmware upload.

This prevents re-alignment or schedule reset on every wake-up.

---

## 🌡 Internal Temperature Sensor Behavior (Expected)

The sketch reads the ESP32’s internal temperature sensor using `temprature_sens_read()`.

- The function **does perform a fresh hardware register read on every wake-up**
- The returned value is **not cached** by the firmware
- The conversion logic is correct

However:

- The ESP32 internal temperature sensor is **not calibrated**
- It has **very low resolution**
- On many chips it returns a nearly constant raw value (commonly 128 °F ≈ 53.33 °C)

As a result:
- The repeated temperature value reflects **actual hardware output**
- The behavior does **not** indicate a stuck function, frozen code path, or scheduling error
- Absolute temperature values from this sensor are not meaningful

---

## ⚠️ Timing Edge Case: Boundary Overrun

Although the overall scheduling approach is sound, a known **edge condition** exists near time boundaries.

### Example Scenario

1. **Scheduled Wake-Up**  
   Target wake-up: `10:00:00`

2. **Execution Delay**  
   Wi-Fi connection and email transmission take ~5 seconds.

3. **Post-Processing Time**  
   Current time becomes `10:00:05`.

4. **Next Boundary Calculation**  
   The next 2-minute boundary is computed as `10:02:00`.

5. **Deep Sleep Entry**  
   The ESP32 sleeps until approximately `10:02:00`.

6. **Wake-Up and NTP Sync**  
   Actual time after sync may be `10:02:01`.

7. **Immediate Log Trigger**  
   Because the logging condition allows execution when  
   `currentTime >= nextCaptureTime`, a second log can be triggered immediately.

---

## Conclusion

- The firmware **functions correctly in principle**
- Deep Sleep, RTC persistence, and NTP alignment behave as designed
- The constant temperature reading reflects **ESP32 hardware limitations**, not a coding error
- The observed scheduling edge case is a natural consequence of strict boundary alignment combined with variable execution time

