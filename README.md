# ESP32 Deep Sleep Time-Aligned Monitoring and Alert System

This repository documents the development of a robust, ultra-low-power environmental monitoring solution using the ESP32. The system is designed to wake up from **Deep Sleep** at precise, clock-aligned intervals (final configuration: **10 minutes**) to read sensor data and transmit it via email (SMTP).

---

## 🚀 Project Goal

The primary objective was to achieve **perfect time-alignment for data logging**. The system had to reliably:

- Enter Deep Sleep for ultra-low power consumption  
- Wake up and synchronise with an NTP server  
- Calculate the exact time remaining until the next scheduled 10-minute mark  
- Send a complete data log via email before returning to sleep  

---

## ⚙️ Iterative Development and Core Challenge

The development process centred on overcoming a persistent and non-trivial scheduling flaw inherent in combining **Deep Sleep** with **NTP synchronisation**.

### The Initial Problem: Continuous Skipping

Early iterations suffered from logs that continuously skipped scheduled intervals.

Example failure mode:
- The ESP32 wakes for the `10:00` log.
- Wi-Fi connection and NTP synchronisation push the current time to `10:00:03`.
- Because the schedule target is `10:00:00`, the system determines it has *missed* the interval.
- The next capture is rescheduled for `10:10:00`, causing the `10:00` log to be skipped entirely.

---

## 🛠️ The Solution: RTC Memory and Safety Buffer

The final, stable implementation introduced **three critical fixes** that fully stabilised the scheduling logic.

### 1. RTC Memory Preservation (`RTC_DATA_ATTR`)

Key variables are stored in RTC memory so they survive Deep Sleep cycles:

- `nextCaptureTime`
- `initialTimeSet`

This ensures the system remembers its intended schedule across wake-ups.

---

### 2. The Schedule Flag (`initialTimeSet`)

A boolean flag ensures that the schedule is calculated **only once**, on the very first boot or firmware flash.

- On first boot: the schedule is initialised
- On subsequent wake-ups: the code skips schedule recalculation and relies on the preserved `nextCaptureTime`

This prevents drift caused by repeatedly re-evaluating timing after each wake cycle.

---

### 3. The 60-Second Safety Buffer

The logging condition was expanded from a strict equality check to include a tolerance window:

Log Condition:
(Current Time ≥ Next Capture Time)
OR
(Time Remaining < 60 seconds)

yaml
Copy code

This buffer guarantees that even if:
- The ESP32 wakes slightly early, or
- Wi-Fi and NTP synchronisation take several seconds

…the system will still execute the log within the valid time window.

---

## ✅ Final Configuration (10-Minute Interval)

The final sketch executes a stable, repeatable, and clock-aligned logging cycle every 10 minutes.

| Feature        | Configuration |
|---------------|---------------|
| Log Interval  | 10 minutes (aligned to :00, :10, :20, etc.) |
| Power State   | Deep Sleep (minimal power draw between logs) |
| Time Source   | NTP (synchronised on every wake-up) |
| Alert Medium  | SMTP Email (Gmail App Password authentication) |

---

## 💡 Use Cases

The reliability and ultra-low-power nature of this logger make it well suited to applications where continuous monitoring is critical but power access is limited—particularly within the **HVAC domain**.

---

## 🔥 Underfloor Heating (UFH) Monitoring (Most Essential)

UFH systems are high-thermal-mass installations with strict safety limits, making precision logging essential.

### Floor Damage Prevention

Most finished floors (e.g., wood, laminate) have a maximum allowable surface temperature (often around **28 °C / 82 °F**).

- If a sensor detects the floor approaching this limit due to a fault (e.g., mixing valve failure),
- The ESP32 sends an **immediate critical email alert**
- Maintenance personnel can shut down boiler flow before irreversible damage occurs (warping, cracking)

---

### Fault Diagnosis via Trending

UFH faults develop slowly due to the thermal inertia of concrete slabs.

- Regular **10-minute email logs** provide clear, verifiable trend data
- A technician can easily identify issues such as:
  - A failed zone actuator
  - A blocked circuit
  - Uneven heat distribution

Archived emails allow fault diagnosis without needing live dashboards.

---

## 🔑 Why Email Notification Is Essential

In critical IoT systems, email is not merely convenient—it is foundational to reliable operation.

### Guaranteed, Unattended Alerts

- Email reliably reaches technicians who are not actively monitoring dashboards
- Standard mobile notifications ensure alerts are seen during off-hours
- Particularly important when protecting high-value UFH installations

---

### Auditable, Verifiable Records

- Emails are archived independently of the sensor device and local network
- Time-stamped logs provide non-erasable proof of system behaviour
- Often required for:
  - Insurance claims
  - Warranty validation
  - Building control or compliance documentation

---

### Ubiquitous Standard

Email is the universal business communication medium.

- No proprietary apps required
- No complex IoT platforms or credentials
- Accessible to any stakeholder using standard tools

This guarantees longevity, accessibility, and operational simplicity.
