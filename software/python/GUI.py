import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os
import sys

# Matplotlib integration for Tkinter
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

from pathFinding import compile_cnc_schedule 

class RedirectConsole:
    def __init__(self, text_widget):
        self.text_widget = text_widget
    def write(self, string):
        self.text_widget.insert(tk.END, string)
        self.text_widget.see(tk.END)
        self.text_widget.update()
    def flush(self):
        pass

class RobotPianoGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Robot Piano CNC Post-Processor")
        self.root.geometry("1000x800")
        
        # --- TABS SETUP ---
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill="both", expand=True, padx=10, pady=10)
        
        self.tab_main = ttk.Frame(self.notebook)
        self.tab_plot = ttk.Frame(self.notebook)
        self.tab_code = ttk.Frame(self.notebook)
        
        self.notebook.add(self.tab_main, text=" Controls & Console ")
        self.notebook.add(self.tab_plot, text=" Kinematics Plot ")
        self.notebook.add(self.tab_code, text=" C-Code Schedule ")
        
        self.setup_main_tab()
        self.setup_code_tab()
        
        self.current_canvas = None # Holds the matplotlib widget

    def setup_main_tab(self):
        # --- 1. SONG SELECTION ---
        frame_file = tk.LabelFrame(self.tab_main, text="1. Select Song (MIDI)", padx=10, pady=10)
        frame_file.pack(fill="x", padx=10, pady=5)
        
        self.filepath_var = tk.StringVar()
        tk.Entry(frame_file, textvariable=self.filepath_var, width=80, state='readonly').pack(side="left", padx=5)
        tk.Button(frame_file, text="Browse...", command=self.browse_file).pack(side="left")

        # --- 2. SOLENOID TOGGLES ---
        frame_config = tk.LabelFrame(self.tab_main, text="2. Right Hand Solenoid Loadout (Max 5)", padx=10, pady=10)
        frame_config.pack(fill="x", padx=10, pady=5)
        
        # Hardcoded floating-point safe offsets
        self.ALL_FINGERS = [
            {'id': 0, 'offset': 6.84, 'type': 'w', 'name': 'Pos 0 (White)'},
            {'id': 1, 'offset': 5.26, 'type': 'b', 'name': 'Pos 1 (Black)'},
            {'id': 2, 'offset': 4.56, 'type': 'w', 'name': 'Pos 2 (White)'},
            {'id': 3, 'offset': 2.98, 'type': 'b', 'name': 'Pos 3 (Black)'},
            {'id': 4, 'offset': 2.28, 'type': 'w', 'name': 'Pos 4 (White)'},
            {'id': 5, 'offset': 0.70, 'type': 'b', 'name': 'Pos 5 (Black)'},
            {'id': 6, 'offset': 0.00, 'type': 'w', 'name': 'Pos 6 (White)'}
        ]
        
        self.finger_vars = {}
        # Layout checkboxes in a grid
        for i, f in enumerate(self.ALL_FINGERS):
            var = tk.IntVar()
            # Default to turning on 0, 1, 3, 4, 6
            if f['id'] in [0, 1, 3, 4, 6]: 
                var.set(1)
                
            cb = tk.Checkbutton(
                frame_config, 
                text=f"{f['name']}  [Offset: {f['offset']}cm]", 
                variable=var, 
                command=lambda fid=f['id']: self.enforce_limit(fid)
            )
            cb.grid(row=i//3, column=i%3, sticky="w", padx=10, pady=5)
            self.finger_vars[f['id']] = var

        # --- 3. GENERATION & CONSOLE ---
        btn_generate = tk.Button(self.tab_main, text="GENERATE C-CODE SCHEDULE", font=("Arial", 12, "bold"), 
                                 bg="#4CAF50", fg="white", command=self.run_compiler)
        btn_generate.pack(fill="x", padx=10, pady=10, ipady=10)

        frame_console = tk.LabelFrame(self.tab_main, text="Compiler Output Console", padx=10, pady=10)
        frame_console.pack(fill="both", expand=True, padx=10, pady=5)
        
        self.text_console = tk.Text(frame_console, bg="black", fg="#00FF00", font=("Courier", 10))
        self.text_console.pack(fill="both", expand=True)
        sys.stdout = RedirectConsole(self.text_console)

    def setup_code_tab(self):
        # --- CODE TAB SCROLLING ---
        scrollbar_code = tk.Scrollbar(self.tab_code)
        scrollbar_code.pack(side="right", fill="y")
        
        self.text_code = tk.Text(self.tab_code, yscrollcommand=scrollbar_code.set, font=("Courier", 10), bg="#1e1e1e", fg="#d4d4d4")
        self.text_code.pack(side="left", fill="both", expand=True, padx=5, pady=5)
        scrollbar_code.config(command=self.text_code.yview)

        # --- PLOT TAB SCROLLING (NEW) ---
        # To make a plot scrollable in Tkinter, we have to put it inside a Canvas
        self.plot_canvas = tk.Canvas(self.tab_plot, bg="white")
        scrollbar_plot = ttk.Scrollbar(self.tab_plot, orient="vertical", command=self.plot_canvas.yview)
        
        self.plot_scrollable_frame = ttk.Frame(self.plot_canvas)
        
        # This tells the canvas to update its scrollable area whenever the plot changes size
        self.plot_scrollable_frame.bind(
            "<Configure>",
            lambda e: self.plot_canvas.configure(scrollregion=self.plot_canvas.bbox("all"))
        )
        
        self.plot_canvas.create_window((0, 0), window=self.plot_scrollable_frame, anchor="nw")
        self.plot_canvas.configure(yscrollcommand=scrollbar_plot.set)
        
        self.plot_canvas.pack(side="left", fill="both", expand=True)
        scrollbar_plot.pack(side="right", fill="y")

    def enforce_limit(self, changed_id):
        """Prevents the user from equipping more than 5 solenoids."""
        active_count = sum(var.get() for var in self.finger_vars.values())
        if active_count > 5:
            messagebox.showwarning("Hardware Limit", "Your carriage can only hold a maximum of 5 solenoids!")
            self.finger_vars[changed_id].set(0) # Uncheck the one they just clicked

    def browse_file(self):
        filepath = filedialog.askopenfilename(filetypes=[("MIDI Files", "*.mid *.midi")])
        if filepath:
            self.filepath_var.set(filepath)

    def run_compiler(self):
        midi_path = self.filepath_var.get()
        if not midi_path:
            messagebox.showerror("Error", "Please select a MIDI file first!")
            return
            
        # Build the configuration list based on what is checked
        active_config = []
        for f in self.ALL_FINGERS:
            if self.finger_vars[f['id']].get() == 1:
                # Strip out the 'name' key so it matches your backend perfectly
                active_config.append({'id': f['id'], 'offset': f['offset'], 'type': f['type']})
            
        self.text_console.delete("1.0", tk.END)
        print("--- INITIATING COMPILATION PIPELINE ---")
        
        try:
            # Call the backend wrapper (which now returns the code and the figure)
            c_code, fig = compile_cnc_schedule(midi_path, active_config)
            
            # --- UPDATE CODE TAB ---
            self.text_code.delete("1.0", tk.END)
            self.text_code.insert(tk.END, c_code)
            
            # --- UPDATE PLOT TAB ---
            # Remove the old plot if it exists
            if self.current_canvas:
                self.current_canvas.get_tk_widget().destroy()
                
            # Inject the plot into the new SCROLLABLE frame instead of the raw tab
            self.current_canvas = FigureCanvasTkAgg(fig, master=self.plot_scrollable_frame)
            self.current_canvas.draw()
            self.current_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
            
            print("\n--- COMPILATION COMPLETE ---")
            print("Check the 'Kinematics Plot' and 'C-Code Schedule' tabs!")
            
            # Automatically switch to the plot tab so the user sees the result!
            self.notebook.select(self.tab_plot)
            
        except Exception as e:
            print(f"\n[!] COMPILER CRASHED: {e}")

if __name__ == "__main__":
    root = tk.Tk()
    app = RobotPianoGUI(root)
    root.mainloop()