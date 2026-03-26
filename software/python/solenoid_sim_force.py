#x(ms^2+Ds+ks) = C*I^2/(g_start-x)^2

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider
from scipy import signal

# --- Fixed System Parameters ---
g0 = 0.02        # Unstretched air gap in meters (20 mm)
C = 0.00005      # Arbitrary Solenoid force constant
D = 5.0          # Damping coefficient (N*s/m)
gravity = 9.81   # Acceleration due to gravity (m/s^2)

# Time vector for simulation (1000 points)
t = np.linspace(0, 2, 1000)

# --- Setup the Figure and Plot ---
fig, ax = plt.subplots(figsize=(10, 7))
plt.subplots_adjust(bottom=0.35)

m_init = 0.5   # Initial Mass (kg)
k_init = 500.0 # Initial Spring constant (N/m)

# FIX: Initialize line with matching dimensions (1000 x points, 1000 y points)
line, = ax.plot(t, np.zeros_like(t), lw=2.5, color='#1f77b4')

ax.set_title("System Step Response (Gravity Modifying Force Gain Only)", fontsize=14)
ax.set_xlabel("Time (seconds)", fontsize=12)
ax.set_ylabel("Dynamic Displacement x(t)", fontsize=12)
ax.grid(True, linestyle='--', alpha=0.7)
ax.set_xlim(0, 2)
ax.set_ylim(-0.01, 0.05)

# Text box for displaying parameters
info_text = ax.text(0.05, 0.85, '', transform=ax.transAxes, fontsize=11,
                    bbox=dict(facecolor='white', alpha=0.8, edgecolor='gray'))

# --- Add Interactive Sliders ---
axcolor = 'lightcyan'
ax_m = plt.axes([0.15, 0.20, 0.7, 0.03], facecolor=axcolor)
ax_k = plt.axes([0.15, 0.10, 0.7, 0.03], facecolor=axcolor)

s_m = Slider(ax_m, 'Mass (kg)', 0.1, 2.0, valinit=m_init, valstep=0.05)
s_k = Slider(ax_k, 'Spring (N/m)', 100.0, 2000.0, valinit=k_init, valstep=10.0)

# --- Update Function ---
def update(val):
    try:
        m = s_m.val
        k = s_k.val
        
        # 1. Calculate the initial stretch from gravity
        delta_x = (m * gravity) / k
        
        # 2. Calculate the new starting air gap
        g_start = g0 - delta_x
        
        # Prevent the gap from going negative if the spring is too weak
        if g_start <= 0.001:
            g_start = 0.001 
            
        # 3. Calculate the Force Multiplier (K) based ONLY on starting gap
        K = C / (g_start**2)
        
        # 4. Standard Mechanical Transfer Function: X(s)/U(s) = K / (m*s^2 + D*s + k)
        sys = signal.TransferFunction([K], [m, D, k])
        _, y = signal.step(sys, T=t)

        # Safety Check: Ignore frame if math results in NaN/Infinity during rapid slider movement
        if np.isnan(y).any() or np.isinf(y).any():
            return

        # FIX: Update both X and Y data explicitly
        line.set_data(t, y)
        
        # Dynamically scale Y-axis safely
        current_max = max(y) if max(y) > 0.01 else 0.01
        if not np.isnan(current_max) and not np.isinf(current_max):
            ax.set_ylim(0, current_max * 1.5)
        
        # Update on-screen text
        text_str = (
            f"Static Stretch ($\\Delta x$): {delta_x*1000:.1f} mm\n"
            f"Starting Gap ($g_{{start}}$): {g_start*1000:.1f} mm\n"
            f"Force Multiplier ($K$): {K:.2f}"
        )
        info_text.set_text(text_str)
        
        fig.canvas.draw_idle()
        
    except Exception as e:
        print(f"Skipped frame due to: {e}")

s_m.on_changed(update)
s_k.on_changed(update)

# Trigger once to draw the initial state
update(0)

plt.show()