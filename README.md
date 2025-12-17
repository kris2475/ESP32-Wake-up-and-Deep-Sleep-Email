# ESP32 Deep Sleep Time-Aligned Logger (10-Minute Interval)

## 📝 Overview

This Arduino sketch runs on an ESP32 microcontroller to periodically read the internal chip temperature and report it via email. The core function of this project is to achieve ultra-low power consumption by utilizing the ESP32's **Deep Sleep** mode while maintaining precise time alignment for data logging.

The system is designed to wake up **exactly on the minute, every 10 minutes** (e.g., `10:00:00`, `10:10:00`, `10:20:00`).

---

## 🔋 Key Design Features

### 1. Low-Power Operation (Deep Sleep)

The primary energy-saving mechanism is **Deep Sleep**.

- The ESP32 is only awake long enough to:
  - Connect to Wi-Fi
  - Synchronize time
  - Read the internal temperature sensor
  - Send an email
- Typical active time: **5–10 seconds**
- Sleep duration: approximately **9 minutes 50 seconds**
- During Deep Sleep, the main CPU is powered down, reducing current consumption to the **microamp range**

This design makes the system suitable for long-term, battery-powered deployments.

---

### 2. Precise Time Alignment (NTP Synchronization)

To prevent time drift over hours or days, the system relies on the **Network Time Protocol (NTP)**.

**Process flow:**

1. **Wake-up**  
   The ESP32 wakes using its internal hardware timer.

2. **Time Synchronization**  
   It immediately connects to Wi-Fi and synchronizes its internal clock with an NTP server.

3. **Calculation**  
   The code calculates the exact number of seconds remaining until the next official 10-minute boundary (e.g., `10:10:00`).

4. **Re-Scheduling**  
   The hardware sleep timer is programmed using this calculated duration.

This guarantees that log timestamps remain aligned with real-world clock time, regardless of:
- Minor hardware timer inaccuracies
- Variable Wi-Fi connection or email transmission delays

---

### 3. Schedule Persistence (RTC Memory)

The sketch uses `RTC_DATA_ATTR` to store variables that persist across Deep Sleep cycles.

**Persistent variables:**

- `nextCaptureTime`  
  Stores the timestamp of the next intended logging event (e.g., `10:10:00`).

- `initialTimeSet`  
  A boolean flag that ensures the schedule is initialized **only on first boot or firmware upload**.

This prevents the logging schedule from being reset or corrupted on every wake-up cycle.

---

## ⚠️ The Flaw in Timing (Scheduling Overrun)

Despite the precise design, a subtle flaw exists in the scheduling logic, observable in log sequences such as:

09:59
10:00

pgsql
Copy code

### Cause of the Flaw: Critical Boundary Condition

The issue occurs when the time spent performing operations pushes execution past a scheduling boundary.

**Example scenario:**

1. **Scheduled Wake-Up**  
   The ESP32 wakes targeting `09:50:00`.

2. **Email Processing Delay**  
   Wi-Fi connection and email transmission take approximately **5 seconds**.

3. **Current Time After Processing**  
   Time is now `09:50:05`.

4. **Next Target Calculation**  
   The code computes the next 10-minute mark based on the current time, resulting in `10:00:00`.

5. **Sleep Duration**  
   The ESP32 sleeps until approximately `10:00:00`.

6. **Wake and Sync**  
   Upon waking and syncing via NTP, the actual time may be `10:00:01`.

7. **Double Log Trigger**  
   Because the system logs when the current time is **greater than or equal to** the scheduled target, the condition is immediately met again, triggering another email at `10:00`.

---

### Summary of the Issue

- The system strictly enforces **real-world time boundaries**
- Small execution delays can push the clock **past the next scheduled interval**
- This results in **two logs firing back-to-back** instead of a full 10-minute sleep cycle

This behavior is **not a failure of the Deep Sleep timer**. It is a side effect of using NTP-synchronized wall-clock time combined with boundary-based scheduling logic.

---

## 📌 Conclusion

This project demonstrates a highly efficient, time-accurate, low-power logging architecture using the ES
