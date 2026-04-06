import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os
import sys
import midi_utils
import pathFinding
from pathFinding import compile_cnc_schedule, plot_piano 
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


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
        # Start in maximized/full screen mode
        self.root.state('zoomed')
        self.original_stdout = sys.stdout
        
        # Set up window close handler
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        # --- TABS SETUP ---
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill="both", expand=True, padx=10, pady=10)
        
        self.tab_main = ttk.Frame(self.notebook)
        self.tab_plot = ttk.Frame(self.notebook)
        self.tab_code = ttk.Frame(self.notebook)
        self.tab_left_hand = ttk.Frame(self.notebook)
        
        self.notebook.add(self.tab_main, text=" Controls & Console ")
        self.notebook.add(self.tab_plot, text=" Kinematics Plot ")
        self.notebook.add(self.tab_code, text=" C-Code Schedule ")
        self.notebook.add(self.tab_left_hand, text=" Left Hand Keys ")
        
        # Status tracking for selected song
        self.optimized = False
        self.c_code_generated = False
        
        self.setup_main_tab()
        self.setup_code_tab()
        self.setup_left_hand_tab()
        
        # Auto-select the songs folder on startup
        self.auto_select_songs_folder()
        
        self.current_canvas = None # Holds the matplotlib widget
        self.header_canvas = None # Holds the header matplotlib widget

    def setup_main_tab(self):
        self.panel_width = 500
        self.song_panel_height = 180
        self.sol_panel_height = 220

        # --- 1. SONG SELECTION ---
        self.song_frame = tk.Frame(self.tab_main)
        self.song_frame.pack(fill="both", expand=True, padx=10, pady=5)

        frame_songsel = tk.LabelFrame(self.song_frame, text="Select Midi File", padx=10, pady=10)
        frame_songsel.pack(side="left", fill="both", expand=True, padx=(0, 5), pady=0)
        frame_songsel.config(width=600, height=self.song_panel_height)

        self.folder_var = tk.StringVar()
        folder_frame = tk.Frame(frame_songsel)
        folder_frame.pack(fill="x", pady=(0, 5))
        tk.Label(folder_frame, text="   Folder:    ").pack(side="left")
        tk.Entry(folder_frame, textvariable=self.folder_var, width=60, state='readonly').pack(side="left", padx=(5, 0))
        tk.Button(folder_frame, text="Browse Folder...", command=self.browse_folder).pack(side="left", padx=(5, 0))

        midi_frame = tk.Frame(frame_songsel, padx=10, pady=5)
        midi_frame.pack(fill="both", expand=True)

        tk.Label(midi_frame, text="MIDI Files Available", font=("Arial", 10, "bold")).pack(anchor="w")
        self.file_var = tk.StringVar()
        self.midi_listbox = tk.Listbox(midi_frame, height=6, width=120, selectmode=tk.SINGLE)
        self.midi_listbox.pack(side="left", padx=(0, 5), pady=(5, 0), fill="y")
        self.midi_listbox.bind('<<ListboxSelect>>', self.on_midi_select)

        scrollbar = tk.Scrollbar(midi_frame)
        scrollbar.pack(side="left", fill="y", pady=(5, 0))
        self.midi_listbox.config(yscrollcommand=scrollbar.set)
        scrollbar.config(command=self.midi_listbox.yview)

        frame_currsong = tk.LabelFrame(self.song_frame, text="Song Status", padx=10, pady=10)
        frame_currsong.pack(side="right", fill="y", padx=(5, 0), pady=0)
        frame_currsong.config(width=self.panel_width, height=self.song_panel_height)
        frame_currsong.pack_propagate(False)

        # Selected song display
        self.selected_song_frame = tk.Frame(frame_currsong, padx=10, pady=10)
        self.selected_song_frame.pack(side="right", fill="y", padx=(5, 0), pady=(0, 0))
        self.selected_song_frame.config(width=self.panel_width, height=160)
        self.selected_song_frame.pack_propagate(False)

        # Song title/status
        self.selected_song_var = tk.StringVar(value="Song: None")
        self.selected_song_label = tk.Label(self.selected_song_frame, textvariable=self.selected_song_var, font=("Arial", 15))
        self.selected_song_label.pack(anchor="nw", pady=(10, 0))

        # Status indicators
        self.optimized_var = tk.StringVar(value="Optimized: No")
        self.optimized_label = tk.Label(self.selected_song_frame, textvariable=self.optimized_var, font=("Arial", 15), fg="red")
        self.optimized_label.pack(anchor="nw", pady=(10, 0))

        self.c_code_var = tk.StringVar(value="C-Code Generated: No")
        self.c_code_label = tk.Label(self.selected_song_frame, textvariable=self.c_code_var, font=("Arial", 15), fg="red")
        self.c_code_label.pack(anchor="nw", pady=(10, 0))


        # --- 2. SOLENOID CONFIGURATION ---
        self.config_frame = tk.Frame(self.tab_main)
        self.config_frame.pack(fill="both", expand=True, padx=10, pady=5)


        # Maximum solenoids setting
        self.max_solenoids = tk.IntVar(value=5)

        # Left side: Manual solenoid loadout
        frame_config = tk.LabelFrame(self.config_frame, text=f"Select Right Hand Solenoid Layout (Max {self.max_solenoids.get()})", padx=10, pady=10)
        frame_config.pack(side="left", fill="both", expand=True, padx=(0, 5), pady=(0,0))
        frame_config.config(width=600, height=self.sol_panel_height)
        frame_config.pack_propagate(False)

        # Max solenoids control
        max_frame = tk.Frame(frame_config)
        max_frame.pack(fill="x", pady=(0, 10))
        tk.Label(max_frame, text="Max Solenoids:").pack(side="left")
        spinbox = tk.Spinbox(max_frame, from_=1, to=7, textvariable=self.max_solenoids, width=3, command=self.update_max_solenoids)
        spinbox.pack(side="left", padx=(5, 0))
        self.max_solenoids.trace_add("write", lambda *args: self.update_max_solenoids())
        
        # Initialize the UI with current max value
        self.update_max_solenoids()

        # Right side: Optimization section
        frame_optimize = tk.LabelFrame(self.config_frame, text="Optimize Solenoid Configuration", padx=10, pady=10)
        frame_optimize.pack(side="right", fill="y", padx=(5, 0), pady=(0,0))
        frame_optimize.config(width=self.panel_width, height=self.sol_panel_height)
        frame_optimize.pack_propagate(False)

        # Keep panel widths and heights stable when the window resizes
        def on_config_frame_resize(event):
            target_left_width = max(event.width - self.panel_width - 10, 300)
            frame_config.config(width=target_left_width, height=max(event.height - 20, self.sol_panel_height))
            frame_optimize.config(width=self.panel_width, height=max(event.height - 20, self.sol_panel_height))

        self.config_frame.bind("<Configure>", on_config_frame_resize)

        # Hardcoded floating-point safe offsets
        self.ALL_FINGERS = [
            {'id': 0, 'offset': 6.84, 'type': 'w', 'name': 'Pos 0 (White)', 'start_pitch': 65},
            {'id': 1, 'offset': 5.70, 'type': 'b', 'name': 'Pos 1 (Black)', 'start_pitch': 66},
            {'id': 2, 'offset': 4.56, 'type': 'w', 'name': 'Pos 2 (White)', 'start_pitch': 67},
            {'id': 3, 'offset': 3.42, 'type': 'b', 'name': 'Pos 3 (Black)', 'start_pitch': 68},
            {'id': 4, 'offset': 2.28, 'type': 'w', 'name': 'Pos 4 (White)', 'start_pitch': 69},
            {'id': 5, 'offset': 1.14, 'type': 'b', 'name': 'Pos 5 (Black)', 'start_pitch': 70},
            {'id': 6, 'offset': 0.00, 'type': 'w', 'name': 'Pos 6 (White)', 'start_pitch': 71}
        ]
                
        self.finger_vars = {}
        self.key_rects = {}  # Store canvas rectangle IDs for visual updates
        
        # Initialize finger states
        for f in self.ALL_FINGERS:
            var = tk.IntVar()

            if f['id'] in [0, 1, 2, 3, 4]: 
                var.set(1)
            self.finger_vars[f['id']] = var
        
        # Create piano keyboard canvas
        self.piano_canvas = tk.Canvas(frame_config, width=600, height=140, bg="#2a2a2a", highlightthickness=0)
        self.piano_canvas.pack(fill="x", padx=5, pady=5)
        self.piano_canvas.bind("<Button-1>", self.on_piano_key_click)
        
        self.draw_piano_keys()

        # Optimization button
        self.btn_optimize = tk.Button(frame_optimize, text=f"FIND OPTIMAL\n{self.max_solenoids.get()}-SOLENOID\nCONFIGURATION", 
                                font=("Arial", 10, "bold"), bg="#4CAF50", fg="white", 
                                command=self.optimize_solenoid_config, height=3)
        self.btn_optimize.pack(fill="x", pady=(0, 10))
        
        # Results display
        self.optimize_result_label = tk.Label(frame_optimize, text="Click button to find\noptimal configuration", 
                                            font=("Arial", 9), justify="center")
        self.optimize_result_label.pack(fill="x")
        
        # Progress indicator
        self.optimize_progress = tk.Label(frame_optimize, text="", font=("Arial", 8), fg="#666666")
        self.optimize_progress.pack(fill="x", pady=(10, 0))

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

        # --- PLOT TAB LAYOUT ---
        # Create a header frame for the frozen piano layout
        self.plot_header_frame = tk.Frame(self.tab_plot, bg="white", height=250)
        self.plot_header_frame.pack(fill="x", padx=0, pady=0)
        self.plot_header_frame.pack_propagate(False)  # Fixed height

        # Create scrollable canvas for the main plot
        self.plot_canvas = tk.Canvas(self.tab_plot, bg="white")
        scrollbar_plot_y = ttk.Scrollbar(self.tab_plot, orient="vertical", command=self.plot_canvas.yview)

        self.plot_scrollable_frame = ttk.Frame(self.plot_canvas)

        # This tells the canvas to update its scrollable area whenever the plot changes size
        self.plot_scrollable_frame.bind(
            "<Configure>",
            lambda e: self.plot_canvas.configure(scrollregion=self.plot_canvas.bbox("all"))
        )

        self.plot_window = self.plot_canvas.create_window((0, 0), window=self.plot_scrollable_frame, anchor="nw")
        self.plot_canvas.configure(yscrollcommand=scrollbar_plot_y.set)

        # Make the frame expand to fill canvas width
        self.plot_canvas.bind(
            "<Configure>",
            lambda e: self.plot_canvas.itemconfig(self.plot_window, width=e.width)
        )

        self.plot_canvas.pack(side="left", fill="both", expand=True)
        scrollbar_plot_y.pack(side="right", fill="y")

    def setup_left_hand_tab(self):
        """Set up the left hand piano keys visualization tab."""
        # Create a frame for the left hand piano
        frame_left_hand = tk.LabelFrame(self.tab_left_hand, text="Left Hand Keys Required", padx=10, pady=10)
        frame_left_hand.pack(fill="both", expand=True, padx=10, pady=5)
        
        # Create piano keyboard canvas for left hand
        self.left_hand_canvas = tk.Canvas(frame_left_hand, width=600, height=140, bg="#2a2a2a", highlightthickness=0)
        self.left_hand_canvas.pack(fill="x", padx=5, pady=5)
        
        # Add instruction text
        tk.Label(frame_left_hand, text="Left hand keys will be analyzed automatically when you generate C-code in the main tab.",
                font=("Arial", 9), fg="#666666").pack(pady=(0, 10))
        
        # Initialize left hand key states
        self.left_hand_keys = {}  # Will store which keys are used

    def auto_select_songs_folder(self):
        """Automatically select the songs folder if it exists."""
        import os
        
        # Get the directory where the script is located
        script_dir = os.path.dirname(os.path.abspath(__file__))
        songs_folder = os.path.join(script_dir, "songs")
        
        # Check if songs folder exists
        if os.path.exists(songs_folder) and os.path.isdir(songs_folder):
            self.folder_var.set(songs_folder)
            self.load_midi_files(songs_folder)
            print(f"Auto-selected songs folder: {songs_folder}")

    def optimize_solenoid_config(self):
        """Find the optimal 5-solenoid configuration for the current MIDI file."""
        import itertools

        midi_path = self.file_var.get() if hasattr(self, 'file_var') and self.file_var.get() else ''
        if not midi_path:
            messagebox.showerror("Error", "Please select a MIDI file first!")
            print("❌ No MIDI file selected for optimization")
            return
        
        # Reset progress
        self.optimize_progress.config(text="Analyzing combinations...")
        self.optimize_result_label.config(text="Testing configurations...\nPlease wait...")
        self.root.update()
        
        try:            
            # Get all possible combinations of max_solenoids fingers from 7
            all_finger_ids = [f['id'] for f in self.ALL_FINGERS]
            max_solenoids = self.max_solenoids.get()
            combinations = list(itertools.combinations(all_finger_ids, max_solenoids))
            
            print(f"\n--- OPTIMIZING SOLENOID CONFIGURATION ---")
            print(f"Testing {len(combinations)} combinations of {max_solenoids} solenoids from {len(all_finger_ids)} available...")
            
            best_config = None
            best_score = float('inf')  # Lower is better (shorter code)
            best_c_code = ""
            
            # Test each combination
            for i, combo in enumerate(combinations):
                self.optimize_progress.config(text=f"Testing combination {i+1}/{len(combinations)}...")
                self.root.update()
                
                # Create configuration for this combination
                active_config = []
                active_fingers = []
                for finger_id in combo:
                    finger = next(f for f in self.ALL_FINGERS if f['id'] == finger_id)
                    active_config.append({
                        'id': finger['id'], 
                        'offset': finger['offset'], 
                        'type': finger['type']
                    })
                    active_fingers.append(finger)

                # Validate with right-hand reachability before executing heavy compile
                unreachable_notes = self.check_right_hand_accessibility(midi_path, active_fingers)
                if unreachable_notes:
                    print(f"Configuration {combo} skipped, missing reach for: {', '.join(unreachable_notes)}")
                    continue

                try:
                    # Run pathfinding for this configuration
                    # Save current working directory since compile_cnc_schedule changes it
                    original_cwd = os.getcwd()
                    _, _, dist = compile_cnc_schedule(midi_path, active_config, 0)
                    os.chdir(original_cwd)  # Restore working directory

                    # Score based on total travel distance (lower is better)
                    score = dist
                    
                    if score < best_score:
                        best_score = score
                        best_config = combo
                        
                except Exception as e:
                    # Skip configurations that fail
                    print(f"Configuration {combo} failed: {e}")
                    continue
            
            if best_config:
                # Update the UI to show the optimal configuration
                self.apply_optimal_config(best_config)
                
                # Display results
                config_names = [next(f['name'] for f in self.ALL_FINGERS if f['id'] == fid) for fid in best_config]
                result_text = f"Optimal Configuration:\n{', '.join(config_names)}\n\nCode length: {best_score} chars"
                self.optimize_result_label.config(text=result_text)
                
                # Update status
                self.optimized = True
                self.optimized_var.set("Optimized: Yes")
                self.optimized_label.config(fg="green")
                
                print(f"✅ Optimization complete!")
                print(f"Best configuration: {best_config}")
                print(f"Configuration names: {config_names}")
                print(f"Code length: {best_score} characters")
                
            else:
                self.optimize_result_label.config(text="No valid configurations found")
                print("❌ No valid configurations found")
            
            self.optimize_progress.config(text="")
            
        except Exception as e:
            self.optimize_result_label.config(text="Optimization failed")
            self.optimize_progress.config(text="")
            print(f"❌ Optimization failed: {e}")

    def apply_optimal_config(self, optimal_combo):
        """Apply the optimal solenoid configuration to the UI."""
        # Update finger variables
        for f in self.ALL_FINGERS:
            self.finger_vars[f['id']].set(f['id'] in optimal_combo)
        
        # Redraw the piano keys
        self.draw_piano_keys()

    def on_closing(self):
        """Handle window close event - cleanup matplotlib figures and exit."""
        import matplotlib.pyplot as plt
        
        # Restore stdout
        sys.stdout = self.original_stdout
        
        # Close all matplotlib figures to free resources
        plt.close('all')
        
        # Destroy the window and quit
        self.root.destroy()

    def draw_piano_keys(self):
        """Draw interactive piano keys on the canvas."""
        self.piano_canvas.delete("all")
        self.key_rects = {}
        
        # Canvas dimensions
        canvas_width = 600
        canvas_height = 140
        
        # Sizing
        white_key_width = 45
        white_key_height = 120
        black_key_width = 28
        black_key_height = 75
        margin_left = 20
        
        # First pass: Draw all white keys
        x_pos = margin_left
        white_key_positions = {}  # Store x_pos for each white key
        for finger in (self.ALL_FINGERS):
            fid = finger['id']
            key_type = finger['type']
            
            if key_type == 'w':
                is_active = self.finger_vars[fid].get() == 1
                
                # White key
                x1 = x_pos
                y1 = 10
                x2 = x_pos + white_key_width
                y2 = y1 + white_key_height
                
                fill_color = "#A1A5F6" if is_active else "#F5F5F5"
                outline_color = "#110F58" if is_active else "#333333"
                
                rect_id = self.piano_canvas.create_rectangle(
                    x1, y1, x2, y2,
                    fill=fill_color,
                    outline=outline_color,
                    width=2,
                    tags=f"key_{fid}"
                )
                
                self.key_rects[fid] = rect_id
                white_key_positions[fid] = (x_pos, x2)
                
                # Move to next position for white keys (they touch)
                x_pos = x2 + 2
        
        # Second pass: Draw all black keys (on top)
        for finger in (self.ALL_FINGERS):
            fid = finger['id']
            key_type = finger['type']
            
            if key_type == 'b':
                is_active = self.finger_vars[fid].get() == 1
                
                # Black key - positioned between white keys, overlapping
                # Find the x position based on the next white key position
                next_white_id = None
                for other_finger in self.ALL_FINGERS:
                    if other_finger['type'] == 'w' and other_finger['id'] > fid:
                        next_white_id = other_finger['id']
                        break
                
                if next_white_id and next_white_id in white_key_positions:
                    x1_white, x2_white = white_key_positions[next_white_id]
                    x_center = x1_white - 1
                else:
                    x_center = x_pos  # Fallback
                
                x1 = x_center - black_key_width // 2
                y1 = 10
                x2 = x_center + black_key_width // 2
                y2 = y1 + black_key_height
                
                fill_color = "#1B1B68" if is_active else "#1a1a1a"
                outline_color = "#080C45" if is_active else "#333333"
                
                rect_id = self.piano_canvas.create_rectangle(
                    x1, y1, x2, y2,
                    fill=fill_color,
                    outline=outline_color,
                    width=2,
                    tags=f"key_{fid}"
                )
                
                self.key_rects[fid] = rect_id
        
        # Add instruction text at the bottom
        self.piano_canvas.create_text(
            canvas_width // 2,
            canvas_height - 10,
            text="Click keys to toggle solenoids",
            fill="#999999",
            font=("Arial", 9)
        )

    def update_max_solenoids(self):
        """Update the frame title and enforce current solenoid count."""
        max_count = self.max_solenoids.get()
        frame_config = None
        for child in self.config_frame.winfo_children():
            if isinstance(child, tk.LabelFrame) and "Right Hand Solenoid Loadout" in child.cget("text"):
                frame_config = child
                break
        
        if frame_config:
            frame_config.config(text=f"2a. Right Hand Solenoid Loadout (Max {max_count})")
        
        # Update optimization button text
        if hasattr(self, 'btn_optimize'):
            self.btn_optimize.config(text=f"FIND OPTIMAL\n{max_count}-SOLENOID\nCONFIGURATION")
        
        # Enforce the limit by deactivating excess solenoids
        if hasattr(self, 'finger_vars'):
            active_count = sum(var.get() for var in self.finger_vars.values())
            if active_count > max_count:
                # Deactivate solenoids starting from the highest ID
                excess = active_count - max_count
                deactivated = 0
                for fid in range(6, -1, -1):  # Start from highest ID
                    if self.finger_vars[fid].get() == 1 and deactivated < excess:
                        self.finger_vars[fid].set(0)
                        deactivated += 1
                        if deactivated >= excess:
                            break
                
                if hasattr(self, 'draw_piano_keys'):
                    self.draw_piano_keys()
                print(f"⚠️ Deactivated {deactivated} solenoid(s) to respect new maximum of {max_count}")

    def reset_optimization_section(self):
        """Reset the optimization panel when the song selection changes."""
        if hasattr(self, 'optimize_result_label'):
            self.optimize_result_label.config(text="Click button to find\noptimal configuration")
        if hasattr(self, 'optimize_progress'):
            self.optimize_progress.config(text="")
        
        # Reset status indicators
        self.optimized = False
        self.c_code_generated = False
        self.optimized_var.set("Optimized: No")
        self.optimized_label.config(fg="red")
        self.c_code_var.set("C-Code Generated: No")
        self.c_code_label.config(fg="red")

    def on_piano_key_click(self, event):
        """Handle piano key clicks. Check black keys first for priority."""
        # First, check black keys (they're on top and should have priority)
        found_key = None
        for finger in self.ALL_FINGERS:
            if finger['type'] == 'b':
                fid = finger['id']
                if fid in self.key_rects:
                    coords = self.piano_canvas.coords(self.key_rects[fid])
                    if coords:
                        x1, y1, x2, y2 = coords
                        if x1 <= event.x <= x2 and y1 <= event.y <= y2:
                            found_key = fid
                            break
        
        # If no black key was clicked, check white keys
        if found_key is None:
            for finger in self.ALL_FINGERS:
                if finger['type'] == 'w':
                    fid = finger['id']
                    if fid in self.key_rects:
                        coords = self.piano_canvas.coords(self.key_rects[fid])
                        if coords:
                            x1, y1, x2, y2 = coords
                            if x1 <= event.x <= x2 and y1 <= event.y <= y2:
                                found_key = fid
                                break
        
        if found_key is not None:
            self.toggle_solenoid(found_key)
            #print(f"Keys selected: {[f['name'] for f in self.ALL_FINGERS if self.finger_vars[f['id']].get() == 1]   }")

    def toggle_solenoid(self, fid):
        """Toggle a solenoid on/off and enforce the limit."""
        current_state = self.finger_vars[fid].get()
        
        # Check if turning on would exceed the limit
        if current_state == 0:
            active_count = sum(var.get() for var in self.finger_vars.values())
            max_allowed = self.max_solenoids.get()
            if active_count >= max_allowed:
                messagebox.showwarning("Hardware Limit", f"Your carriage can only hold a maximum of {max_allowed} solenoids!")
                print(f"⚠️ Cannot activate more than {max_allowed} solenoids at once!")
                return

        # Toggle the state
        self.finger_vars[fid].set(1 - current_state)
        
        # Redraw the piano to show the new state
        self.draw_piano_keys()

    def draw_left_hand_piano(self):
        """Draw the left hand piano keys showing which ones are used."""
        self.left_hand_canvas.delete("all")
        
        # Canvas dimensions
        canvas_width = 600
        canvas_height = 140
        
        # Define the left hand key layout (C2 to B2)
        LEFT_HAND_KEYS = [
            {'id': 0, 'pitch': 48, 'name': 'C2', 'type': 'w'},
            {'id': 1, 'pitch': 49, 'name': 'C#2', 'type': 'b'},
            {'id': 2, 'pitch': 50, 'name': 'D2', 'type': 'w'},
            {'id': 3, 'pitch': 51, 'name': 'D#2', 'type': 'b'},
            {'id': 4, 'pitch': 52, 'name': 'E2', 'type': 'w'},
            {'id': 5, 'pitch': 53, 'name': 'F2', 'type': 'w'},
            {'id': 6, 'pitch': 54, 'name': 'F#2', 'type': 'b'},
            {'id': 7, 'pitch': 55, 'name': 'G2', 'type': 'w'},
            {'id': 8, 'pitch': 56, 'name': 'G#2', 'type': 'b'},
            {'id': 9, 'pitch': 57, 'name': 'A2', 'type': 'w'},
            {'id': 10, 'pitch': 58, 'name': 'A#2', 'type': 'b'},
            {'id': 11, 'pitch': 59, 'name': 'B2', 'type': 'w'},
            {'id': 12, 'pitch': 60, 'name': 'C3', 'type': 'w'},
            {'id': 13, 'pitch': 61, 'name': 'C#3', 'type': 'b'},
            {'id': 14, 'pitch': 62, 'name': 'D3', 'type': 'w'},
            {'id': 15, 'pitch': 63, 'name': 'D#3', 'type': 'b'},
            {'id': 16, 'pitch': 64, 'name': 'E3', 'type': 'w'}
        ]
        
        # Sizing
        white_key_width = 40
        white_key_height = 120
        black_key_width = 24
        black_key_height = 75
        margin_left = 20
        
        # First pass: Draw all white keys
        x_pos = margin_left
        white_key_positions = {}
        for key in LEFT_HAND_KEYS:
            if key['type'] == 'w':
                is_used = key['id'] in self.left_hand_keys
                
                # White key
                x1 = x_pos
                y1 = 10
                x2 = x_pos + white_key_width
                y2 = y1 + white_key_height
                
                fill_color = "#A1A5F6" if is_used else "#F5F5F5"
                outline_color = "#110F58" if is_used else "#333333"

                
                self.left_hand_canvas.create_rectangle(
                    x1, y1, x2, y2,
                    fill=fill_color,
                    outline=outline_color,
                    width=2
                )
                
                # Add key label
                self.left_hand_canvas.create_text(
                    (x1 + x2) // 2, y1 + white_key_height * 3 // 4,
                    text=key['name'],
                    fill="#000000" if is_used else "#666666",
                    font=("Arial", 8, "bold")
                )
                
                white_key_positions[key['id']] = (x_pos, x2)
                
                # Move to next position
                x_pos = x2 + 2
        
        # Second pass: Draw all black keys (on top)
        for key in LEFT_HAND_KEYS:
            if key['type'] == 'b':
                is_used = key['id'] in self.left_hand_keys
                
                # Find position between white keys
                prev_white_id = None
                next_white_id = None
                for other_key in LEFT_HAND_KEYS:
                    if other_key['type'] == 'w':
                        if other_key['id'] < key['id']:
                            prev_white_id = other_key['id']
                        elif other_key['id'] > key['id'] and next_white_id is None:
                            next_white_id = other_key['id']
                
                if next_white_id and next_white_id in white_key_positions:
                    x1_white, x2_white = white_key_positions[next_white_id]
                    x_center = x1_white - 1
                else:
                    continue
                
                x1 = x_center - black_key_width // 2
                y1 = 10
                x2 = x_center + black_key_width // 2
                y2 = y1 + black_key_height
                

                fill_color = "#1B1B68" if is_used else "#1a1a1a"
                outline_color = "#080C45" if is_used else "#333333"
                
                self.left_hand_canvas.create_rectangle(
                    x1, y1, x2, y2,
                    fill=fill_color,
                    outline=outline_color,
                    width=2
                )
                
                # Add key label for black keys
                if is_used:
                    self.left_hand_canvas.create_text(
                        (x1 + x2) // 2, y1 + black_key_height // 2,
                        text=key['name'],
                        fill="#FFFFFF",
                        font=("Arial", 7, "bold")
                    )
        
        # Add instruction text at the bottom
        # self.left_hand_canvas.create_text(
        #     canvas_width // 2,
        #     canvas_height - 10,
        #     text="Left hand keys required for this song (highlighted = used)",
        #     fill="#999999",
        #     font=("Arial", 9)
        # )

    def browse_folder(self):
        folder_path = filedialog.askdirectory()
        if folder_path:
            self.folder_var.set(folder_path)
            self.load_midi_files(folder_path)

    def load_midi_files(self, folder_path):
        """Load all MIDI files from the selected folder."""
        import os
        self.midi_listbox.delete(0, tk.END)
        self.midi_files = []
        
        try:
            for file in os.listdir(folder_path):
                if file.lower().endswith(('.mid', '.midi')):
                    self.midi_listbox.insert(tk.END, file)
                    self.midi_files.append(os.path.join(folder_path, file))
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load MIDI files: {e}")

    def on_midi_select(self, event):
        """Handle MIDI file selection from listbox."""
        selection = self.midi_listbox.curselection()
        if selection:
            index = selection[0]
            if index < len(self.midi_files):
                selected_path = self.midi_files[index]
                self.file_var.set(selected_path)
                self.selected_song_var.set(f"Song: {os.path.basename(selected_path)}")
                self.reset_optimization_section()
                print(f"Selected MIDI file: {selected_path}")

    def check_right_hand_accessibility(self, midi_path, active_fingers):
        """Return right-hand inaccesssible notes for active solenoid fingers."""
        # Set working directory like compile_cnc_schedule does
        target_dir = os.path.dirname(os.path.dirname(midi_path))
        original_cwd = os.getcwd()
        os.chdir(target_dir)
        
        try:
            song_name = os.path.splitext(os.path.basename(midi_path))[0]
            notes = midi_utils.midi_to_notes(song_name)

            right_hand_notes = [n for n in notes if pathFinding.RH_MIN_PITCH <= n.pitch <= pathFinding.RH_MAX_PITCH]
            right_hand_required = set(n.pitch for n in right_hand_notes)

            inaccessible_notes = []
            for pitch in sorted(right_hand_required):
                note_is_black = pathFinding.is_black_key(pitch)
                note_has_access = False
                for finger in active_fingers:
                    if note_is_black and finger['type'] == 'b' and finger['start_pitch'] <= pitch:
                        note_has_access = True
                        break
                    elif not note_is_black and finger['type'] == 'w' and finger['start_pitch'] <= pitch:
                        note_has_access = True
                        break

                if not note_has_access:
                    note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
                    name = f"{note_names[pitch % 12]}{pitch // 12 - 1}"
                    inaccessible_notes.append(name)

            return inaccessible_notes
        finally:
            # Always restore original working directory
            os.chdir(original_cwd)

    def browse_file(self):
        filepath = filedialog.askopenfilename(filetypes=[("MIDI Files", "*.mid *.midi")])
        if filepath:
            self.filepath_var.set(filepath)

    def run_compiler(self):
        midi_path = self.file_var.get() if hasattr(self, 'file_var') and self.file_var.get() else ''
        if not midi_path:
            messagebox.showerror("Error", "Please select a MIDI file first!")
            print("❌ No MIDI file selected for compilation")
            return
            
        # Build the configuration list based on what is checked
        active_config = []
        for f in self.ALL_FINGERS:
            if self.finger_vars[f['id']].get() == 1:
                # Strip out the 'name' key so it matches your backend perfectly
                active_config.append({'id': f['id'], 'offset': f['offset'], 'type': f['type']})
            
        print("\n--- INITIATING COMPILATION PIPELINE ---")
        
        try:
            # Call the backend wrapper (which now returns the code and the figure)
            c_code, fig, _ = compile_cnc_schedule(midi_path, active_config, 1)
            
            # --- UPDATE CODE TAB ---
            self.text_code.delete("1.0", tk.END)
            self.text_code.insert(tk.END, c_code)
            
            # Update status
            self.c_code_generated = True
            self.c_code_var.set("C-Code Generated: Yes")
            self.c_code_label.config(fg="green")
            
            # --- UPDATE PLOT TAB ---
            # Remove the old plot if it exists
            if self.current_canvas:
                self.current_canvas.get_tk_widget().destroy()
            if self.header_canvas:
                self.header_canvas.get_tk_widget().destroy()
                
            # Make the plot figure fit the available canvas width before embedding
            canvas_width = self.plot_canvas.winfo_width() if self.plot_canvas.winfo_width() > 0 else 800
            dpi = fig.get_dpi() if hasattr(fig, 'get_dpi') else 100
            fig.set_size_inches(canvas_width / dpi, fig.get_size_inches()[1], forward=True)

            # --- CREATE HEADER (Piano Layout) ---
            # Use plot_piano function to create the header
            header_fig = plot_piano(print_plot=0)  # Get figure without showing
            
            # Embed the matplotlib figure directly in the header frame
            self.header_canvas = FigureCanvasTkAgg(header_fig, master=self.plot_header_frame)
            self.header_canvas.draw()
            self.header_canvas.get_tk_widget().pack(fill=tk.X, padx=(50, 20), pady=0, expand=False)

            # Inject the plot into the scrollable frame
            self.current_canvas = FigureCanvasTkAgg(fig, master=self.plot_scrollable_frame)
            self.current_canvas.draw()
            self.current_canvas.get_tk_widget().pack(fill=tk.X, expand=True)

            # --- RIGHT HAND ACCESSIBILITY CHECK ---
            print("\n--- ANALYZING RIGHT HAND KEYS ---")

            # Determine active fingers from the GUI solenoid configuration
            active_fingers = [f for f in self.ALL_FINGERS if self.finger_vars[f['id']].get() == 1]
            inaccessible_notes = self.check_right_hand_accessibility(midi_path, active_fingers)

            if inaccessible_notes:
                print(f"⚠️  Right-hand notes required by song but not reachable with selected solenoids: {', '.join(inaccessible_notes)}")
            else:
                print("✅ All required right-hand notes are reachable with current solenoid loadout.")

            
            # --- ANALYZE LEFT HAND KEYS ---
            print("\n--- ANALYZING LEFT HAND KEYS ---")
            try:
                song_name = os.path.splitext(os.path.basename(midi_path))[0]
                notes = midi_utils.midi_to_notes(song_name)

                # Separate left and right hand notes
                left_hand_notes = [n for n in notes if n.pitch <= pathFinding.LH_MAX_PITCH]
                left_pitches = [note.pitch for note in left_hand_notes]
                
                # print(f"Left hand pitches: {left_pitches}")

                # Count frequency of each pitch
                pitch_counts = {}
                for pitch in left_pitches:
                    pitch_counts[pitch] = pitch_counts.get(pitch, 0) + 1

                # Determine which keys are used based on the LEFT_FINGERS mapping
                self.left_hand_keys = {}
                for pitch, count in pitch_counts.items():
                    if pitch in pathFinding.LEFT_FINGERS:
                        finger_id = pathFinding.LEFT_FINGERS[pitch]
                        self.left_hand_keys[finger_id] = count
                
                # print(f"Left hand fingers: {pathFinding.LEFT_FINGERS}")

                # Draw the left hand piano
                self.draw_left_hand_piano()
                
                print(f"✅ Left hand analysis complete! Found {len(self.left_hand_keys)} unique keys used.")
                
            except Exception as e:
                print(f"❌ Error analyzing left hand keys: {e}")
            
            print("\n--- COMPILATION COMPLETE ---")
            print("Check the 'Kinematics Plot', 'C-Code Schedule', and 'Left Hand Keys' tabs!\n")
            
            
        except Exception as e:
            print(f"\n[!] COMPILER CRASHED: {e}\n")
            # Reset C-code status on failure
            self.c_code_generated = False
            self.c_code_var.set("C-Code Generated: No")
            self.c_code_label.config(fg="red")

if __name__ == "__main__":
    root = tk.Tk()
    app = RobotPianoGUI(root)
    root.mainloop()
    import sys
    sys.exit(0)