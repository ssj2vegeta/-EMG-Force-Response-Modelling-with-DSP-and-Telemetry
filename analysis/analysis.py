"""
EMG-Force characterisation
"""

import pandas as pd
import numpy as np
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt



CSV_FILE = "session_features.csv"

# Note - Fill in from notebook after session
trials = [
    {"force": 20, "rep": 1, "start_ms": 0,  "end_ms": 0},
    {"force": 20, "rep": 2, "start_ms": 0,  "end_ms": 0},
    {"force": 20, "rep": 3, "start_ms": 0,  "end_ms": 0},
    {"force": 40, "rep": 1, "start_ms": 0,  "end_ms": 0},
    {"force": 40, "rep": 2, "start_ms": 0,  "end_ms": 0},
    {"force": 40, "rep": 3, "start_ms": 0,  "end_ms": 0},
    {"force": 60, "rep": 1, "start_ms": 0,  "end_ms": 0},
    {"force": 60, "rep": 2, "start_ms": 0,  "end_ms": 0},
    {"force": 60, "rep": 3, "start_ms": 0,  "end_ms": 0},
    {"force": 75, "rep": 1, "start_ms": 0,  "end_ms": 0},
    {"force": 75, "rep": 2, "start_ms": 0,  "end_ms": 0},
    {"force": 75, "rep": 3, "start_ms": 0,  "end_ms": 0},
]


df = pd.read_csv(CSV_FILE)
rms_df = df[df["type"] == "rms"].copy()

def extract_trial_mean(rms_df, trial):
    start = trial["start_ms"] + 2500   
    end   = trial["end_ms"]   - 2500   
    window = rms_df[
        (rms_df["timestamp_ms"] >= start) &
        (rms_df["timestamp_ms"] <= end)
    ]
    if len(window) == 0:
        return None
    return window["value"].mean()

results = {}

for trial in trials:
    mean_rms = extract_trial_mean(rms_df, trial)
    if mean_rms is None:
        print(f"Warning: no data for force={trial['force']} rep={trial['rep']}")
        continue
    force = trial["force"]
    if force not in results:
        results[force] = []
    results[force].append(mean_rms)

force_levels  = sorted(results.keys())
mean_rms_arr  = np.array([np.mean(results[f]) for f in force_levels])
se_rms_arr    = np.array([np.std(results[f], ddof=1) / np.sqrt(len(results[f]))
                           for f in force_levels])
force_arr     = np.array(force_levels, dtype=float)

print("\n Per force level")
for f, m, s in zip(force_levels, mean_rms_arr, se_rms_arr):
    print(f"  {f}% MVC:  mean RMS = {m:.6f}  SE = {s:.6f}")

def power_law(x, a, b):
    return a * np.power(x, b)

def linear(x, a, b):
    return a * x + b

popt_pow, pcov_pow = curve_fit(power_law, force_arr, mean_rms_arr,
                                sigma=se_rms_arr, absolute_sigma=True,
                                p0=[0.001, 1.0])

popt_lin, pcov_lin = curve_fit(linear, force_arr, mean_rms_arr,
                                sigma=se_rms_arr, absolute_sigma=True,
                                p0=[0.001, 0.0])

a_pow, b_pow = popt_pow
a_lin, b_lin = popt_lin

print(f"\n Fitted parameters ")
print(f"  Power law:  a = {a_pow:.6f},  b = {b_pow:.4f}")
print(f"  Linear:     a = {a_lin:.6f},  b = {b_lin:.6f}")

n_bootstrap = 1000
b_values = []

for _ in range(n_bootstrap):
    idx    = np.random.choice(len(force_arr), size=len(force_arr), replace=True)
    x_boot = force_arr[idx]
    y_boot = mean_rms_arr[idx]
    try:
        popt_boot, _ = curve_fit(power_law, x_boot, y_boot, p0=[0.001, 1.0])
        b_values.append(popt_boot[1])
    except RuntimeError:
        pass

b_ci = np.percentile(b_values, [2.5, 97.5])
print(f"\n Bootstrap 95% CI on b ")
print(f"  b = {b_pow:.4f}  [{b_ci[0]:.4f}, {b_ci[1]:.4f}]")
print(f"  Huang et al. distal biceps: b = 1.04 ± 0.14  [0.90, 1.18]")

def aic(n, k, residuals):
    ssr = np.sum(residuals ** 2)
    return 2 * k + n * np.log(ssr / n)

n            = len(force_arr)
residuals_pow = mean_rms_arr - power_law(force_arr, *popt_pow)
residuals_lin = mean_rms_arr - linear(force_arr, *popt_lin)

aic_pow = aic(n, 2, residuals_pow)
aic_lin = aic(n, 2, residuals_lin)
preferred = "power law" if aic_pow < aic_lin else "linear"

print(f"\n AIC model comparison ")
print(f"  AIC power law: {aic_pow:.4f}")
print(f"  AIC linear:    {aic_lin:.4f}")
print(f"  Preferred: {preferred}  (ΔAIC = {abs(aic_pow - aic_lin):.4f})")

def r_squared(y_actual, y_predicted):
    ss_res = np.sum((y_actual - y_predicted) ** 2)
    ss_tot = np.sum((y_actual - y_actual.mean()) ** 2)
    return 1 - (ss_res / ss_tot)

r2_pow = r_squared(mean_rms_arr, power_law(force_arr, *popt_pow))
r2_lin = r_squared(mean_rms_arr, linear(force_arr, *popt_lin))

print(f"\n R² ")
print(f"  Power law: R² = {r2_pow:.4f}")
print(f"  Linear:    R² = {r2_lin:.4f}")

x_fit = np.linspace(min(force_arr) * 0.9, max(force_arr) * 1.1, 200)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

ax1.errorbar(force_arr, mean_rms_arr, yerr=se_rms_arr,
             fmt="o", color="black", capsize=4, label="data (mean ± SE)")
ax1.plot(x_fit, power_law(x_fit, *popt_pow), "b-",
         label=f"power law  b={b_pow:.3f} [{b_ci[0]:.3f}, {b_ci[1]:.3f}]")
ax1.plot(x_fit, linear(x_fit, *popt_lin), "r--",
         label=f"linear  R²={r2_lin:.3f}")
ax1.set_xlabel("Force (%MVC)")
ax1.set_ylabel("EMG RMS")
ax1.set_title("EMG–Force Relationship")
ax1.legend(fontsize=8)
ax1.grid(True, alpha=0.3)

ax2.scatter(force_arr, residuals_pow, color="blue", label="power law residuals")
ax2.scatter(force_arr, residuals_lin, color="red", marker="x", label="linear residuals")
ax2.axhline(0, color="grey", linestyle="--")
ax2.set_xlabel("Force (%MVC)")
ax2.set_ylabel("Residual")
ax2.set_title("Residuals")
ax2.legend(fontsize=8)
ax2.grid(True, alpha=0.3)

fig.suptitle(f"EMG–Force  |  b = {b_pow:.3f} [{b_ci[0]:.3f}, {b_ci[1]:.3f}]  |  {preferred} preferred (AIC)")
fig.tight_layout()
plt.savefig("emg_force_results.png", dpi=150)
plt.show()
