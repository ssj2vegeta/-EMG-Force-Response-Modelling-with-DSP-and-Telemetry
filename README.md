# EMG-Force Characterisation — Real-Time Acquisition, Modelling and Live Plotting

## Overview

This project sits at the intersection of my physics background and my interest in hardware, software, and quantitative research — building a full real-time EMG acquisition and analysis pipeline and compared with peer-reviewed literature, on an ESP32 microcontroller.

There is no universally confirmed mathematical EMG-force relationship. I've tried to model it for biceps brachii during isometric contractions, fitting linear and power-law models and comparing the fitted exponent b to published values from Huang et al. (2023). Even that paper makes simplifying assumptions — the simulation uses a cylindrical muscle model (Fuglevand model) with three motor unit depth conditions to explain the spatial dependence of b across the electrode array. The real biceps is not a cylinder. That tension between a proposed model with simplified assumptions and reality makes this interesting. 


**Hardware — TO DO**
Add wiring table, photo of setup, MyoWare specs, electrode placement photo.

## Debugging (Hardware)

- AD8232 bring-up attempted with 3 modules, 2 ESP32 boards, 2 breadboards
- Systematic debugging: verified ESP32 ADC functional, identified LO+ floating issue, measured 0.4V output vs expected 1.65V
- Root cause: Solder Iron too hot (500 C) 150 C over recommended limit for boards.
- Decision: switched to MyoWare 2.0 — changed only the analog frontend (different filters) but objectives still the same/


## ESP32 Firmware 

**Firmware architecture (ESP-IDF / FreeRTOS)**

Four tasks: sampling ISR at highest priority, all other tasks at lower priority. Sampling and DSP pinned to core 1. WiFi and MQTT pinned to core 0 — physically isolating the sampling/dsp tasks from mqtt/wifi which are resource intensive. 

The sample queue between the ISR and DSP task is key — the ISR never blocks waiting for DSP processing, drops the sample and returns immediately. 

**DSP pipeline:** DC removal (exponential running mean, removes baseline drift), rectification (prepares signal for amplitude estimation), RMS windowing (250ms sliding window, standard amplitude measure in EMG literature — Lawrence & De Luca, 1983).

**Network:** WiFi STA mode with auto-reconnect. MQTT to local Mosquitto broker. Two topics published at 10Hz — raw rectified ADC values and RMS features, both as JSON. LWT registered on connect so the broker publishes an offline status message on unexpected disconnect.



## Python Pipeline


**Python pipeline**

Two scripts run from the `analysis/` directory.

**session.py** — run during the experiment. Connects to the MQTT broker, subscribes to both topics, logs all incoming messages to a timestamped CSV , and renders a live scrolling plot of raw and RMS traces side by side. MQTT loop runs in a background thread leaving the main thread free for matplotlib. 

**analysis.py** — run after the session. Reads the CSV, extracts the middle 5 seconds of each trial, aggregates across 3 reps per force level to get mean RMS and standard error, fits linear and power-law models using weighted least squares using scipy, computes bootstrap CI on the power-law exponent b, runs AIC model comparison, and generates results plots.


## Signal Acquisition


**Signal acquisition:**

MyoWare 2.0 muscle sensor, RECT output (200x gain, 20–498Hz bandpass, hardware full-wave rectification). Sampled at 1kHz via ESP32 ADC (12-bit, GPIO35). Electrode placement following SENIAM guidelines for biceps brachii — 1/3 of the distance from the cubital fossa toward the acromion, aligned along the muscle fibre direction. Reference electrode on wrist.

**DSP pipeline (firmware):** DC removal via exponential running mean, rectification (redundant for RECT output but harmless), 250ms RMS sliding window. Output published over MQTT at 10Hz.

## Experimental Protocol

**Experimental protocol:** Isometric bicep curls at 20, 40, 60, and 75% MVC. MVC estimated from maximum dumbbell mass and forearm moment arm. Three repetitions per force level, 10s holds, 30s rest between repetitions. Trial timestamps noted from terminal output during session.

**Analysis:** Middle 5 seconds of each hold extracted to avoid onset/offset transients. Three reps per force level averaged to give mean RMS and standard error. Linear and power-law models fitted using weighted least squares (scipy.optimize.curve_fit, sigma=SE, absolute_sigma=True). Bootstrap CI on power-law exponent b (1000 resamples, 2.5th–97.5th percentiles). AIC used for model comparison. Fitted b compared against Huang et al. (2023) distal biceps value of 1.04 ± 0.14.



**Results — TO DO**
Run experimental sessions, fill in timestamps, run analysis.py, paste fitted b value, bootstrap CI, AIC result, and save emg_force_results.png.


## Limitations

- Electrical noise from laptop power supply — MyoWare is sensitive enough to pick up mains noise when the laptop is plugged into a wall outlet. All sessions conducted on battery power only. A USB isolator is recommended for full isolation but was not used. Wireless keyboard and mouse recommended during recording as the MyoWare can pick up trackpad and keyboard electrical activity causing false reading

- MyoWare 2.0 RECT output rather than raw bipolar EMG — hardware rectification differs slightly from software rectification used by Huang et al.

- Only one subject (me)

- Torque equation + dumbells is not as reliable as cable sensor in Huang et al.
  

## References

**Lawrence, J.H. & De Luca, C.J. (1983).** Myoelectric signal versus force relationship in different human muscles. https://pubmed.ncbi.nlm.nih.gov/6874489/

**Huang, Y. et al. (2023).** Spatial distribution of the EMG–force relationship. https://doi.org/10.3390/bioengineering10040469

**Hermens, H.J. et al. (2000).** SENIAM guidelines for surface EMG electrode placement. http://www.seniam.org/
