import pretty_midi
import midi_utils
import collections
import math

WHITE_KEY_WIDTH_CM = 2.0 
HIT_TOLERANCE_CM = 0.3 

# RH - LF Split
SPLIT_POINT = 48

# offset is distance in cm from the  left edge)
# 'type': 'w' for White Key Finger, 'b' for Black Key Finger.
ROBOT_FINGERS = [
    {'id': 0, 'offset': 0.0, 'type': 'w'},  
    {'id': 1, 'offset': 1.0, 'type': 'b'}, 
    {'id': 2, 'offset': 2.0, 'type': 'w'}, 
    {'id': 3, 'offset': 3.0, 'type': 'b'},  
    {'id': 4, 'offset': 4.0, 'type': 'w'},
]

def is_black_key(midi_pitch):
    """Returns True if the MIDI pitch corresponds to a black key."""
    # Black keys: 1(C#), 3(D#), 6(F#), 8(G#), 10(A#)
    return (midi_pitch % 12) in [1, 3, 6, 8, 10]


"""
midi_pitch: <int> pitch of specific note
Function used to conver pitch into cm (might need to double check this I did a general case unsure if this matches exactly the midi library)
"""
def get_note_position_cm(midi_pitch):
    # Relative offsets from the start of the octave (C=0)
    octave_offsets = {
        0: 0.0, 1: 0.5, 2: 1.0, 3: 1.5, 4: 2.0, 
        5: 3.0, 6: 3.5, 7: 4.0, 8: 4.5, 9: 5.0, 10: 5.5, 11: 6.0
    }
    
    #Gives the octave (1-7)
    pitch_diff = midi_pitch - SPLIT_POINT
    octave = (pitch_diff // 12) - 1
    #Mod 12 gives the specific note
    note_in_octave = pitch_diff % 12
    
    index = (octave * 7) + octave_offsets[note_in_octave]
    return index * WHITE_KEY_WIDTH_CM


"""Basic Physics simulation to estimate time.
dist_cm: Distance hand will need to travel to reach position

Add-ons 
1. Function for acceleration instead of strict time penalty
    a. Eventually max velocity is hit so would an if else statement or a piecewise
2. Initial wind up time for torque to start moving (not sure if thats how the physics works ask George)
""" 
def get_travel_time(dist_cm):

    if dist_cm == 0: return 0.0
    max_velocity = 60.0 # cm/s
  #  acceleration_penalty = 0.05 # time to accelerate maybe make this into a function ? Just hardcoding a time penalty for time being
    return (dist_cm / max_velocity) #+ acceleration_penalty


""" 
Finds all hand positions where the robot can play all the notes in the timestamp.
chord_data: [(note_pos_cm, is_black?)]
"""
    
def get_valid_hand_positions(chord_data):

    if not chord_data: return []
    
    valid_hs = []
    potential_hs = set()
    
    # Try to align every COMPATIBLE finger with every note
    for note_pos, note_is_black in chord_data:
        for finger in ROBOT_FINGERS:
            
            #Limitation due to finger types essentially doing a equivalency check:
            #Black note = Black finger? Possible hand position
            #White Note = White finger? Possible hand position
            #if nothing is found its unplayable
            finger_is_black = (finger['type'] == 'b')
            if note_is_black == finger_is_black:
                # hand_position = Note - Finger_Offset
                potential_hs.add(note_pos - finger['offset'])
            
    # Check if a candidate position 'h' allows us to hit every note in the chord / timestamp
    for h in potential_hs:
        chord_covered = True
        
        for note_pos, note_is_black in chord_data:
            note_hit = False
            
            # Search for any finger on the hand that hits this specific note
            for finger in ROBOT_FINGERS:
                # Check for type as only black fingers can hit black keys
                finger_is_black = (finger['type'] == 'b')
                if note_is_black != finger_is_black:
                    continue #Go to next finger as this isnt a valid position

                # Check For distance, ideally this hits 0 tolerance is there as a parameter we can adjust in the future
                finger_loc = h + finger['offset']
                if abs(note_pos - finger_loc) <= HIT_TOLERANCE_CM:
                    note_hit = True
                    break 
            
            if not note_hit:
                chord_covered = False
                break # This hand position failed to cover the chord (can remove it from our valid hand positions)
        
        if chord_covered:
            valid_hs.append(h)
            
    return sorted(list(valid_hs))


"""
Main Algorithm for finding the best time path minimizing time error ( require_time - time_start - prev_time)
[notes: (start=<time>, end=<time>, pitch=<int>, velocity=?)
"""
def find_best_time_path(notes):
    
    #Split hand function so filter RH/LH notes
    right_hand_notes = [n for n in notes if n.pitch >= SPLIT_POINT]
    
    #Avoids errors for unmatch keys
    #groups: [time: [pitch1, pitch...]]
    groups = collections.defaultdict(list)
    for note in right_hand_notes:
        groups[note.start].append(note.pitch)
        
    #Sort chord by pitches
    sorted_times = sorted(groups.keys())
    
    # Convert chords into time_steps_data-data: [(position of note (cm), Is_Black_Key ? ), ...]
    time_steps_data = []
    for t in sorted_times:
        raw_pitches = groups[t]
        step_data = []
        for p in raw_pitches:
            pos = get_note_position_cm(p)
            is_b = is_black_key(p)
            step_data.append((pos, is_b))
        
        #Additional sort just to make sure
        #Check position 0 of each list (position_in_cm, true/false) and organize from lowest to highest
        step_data.sort(key=lambda x: x[0])
        if step_data:
            time_steps_data.append(step_data)

    #If notes is empty just return an empty list 
    if not time_steps_data: return [], []


    #Main optimization algorithm
    layers = []
    path_traceback = []
    
    # start: Assume 0 cost to start at any valid position -> Can append this eventually as we have homing in place but not really sure where we are homing from and im tired
    first_candidates = get_valid_hand_positions(time_steps_data[0])
    
    # fallback for starting position: If the very first chord is impossible (spans too wide), start at 0cm (Eventually this impossibility might need to be replaced with rearranging the chord into inversions or simply removing notes?)
    if not first_candidates: first_candidates = [0.0]
        
    layers.append({pos: 0.0 for pos in first_candidates})
    path_traceback.append({})

    # viterbi loop (I miss 259)
    
    """
    
    layers = [
    # LAYER 0 (start)
    # Robot could start at 50cm or 60cm with 0 error.
    { 50.0: 0.0,  60.0: 0.0 },

    # LAYER 1 (1st move)
    # Moving to 52cm cost 0.1s error. Moving to 62cm cost 0.05s error.
    { 52.0: 0.1,  62.0: 0.05 },

    # LAYER 2 (2nd move)
    # The error accumulates.
    { 55.0: 0.15, 65.0: 0.05 } 
    
    ....        
]

    """
    for i in range(1, len(time_steps_data)):
        current_chord = time_steps_data[i]
        
        # calculate time between notes
        current_time = sorted_times[i]
        prev_time = sorted_times[i-1]
        available_time = current_time - prev_time
        
        candidates = get_valid_hand_positions(current_chord)
        
        current_layer_cost = {}
        current_layer_parents = {}
        
        prev_layer = layers[i-1]
        
        # If impossible chord, stay in place (spans too wide) same as before might want to rework this
        if not candidates:
            layers.append(prev_layer)
            path_traceback.append({k:k for k in prev_layer})
            continue

        # Check transitions every possible way to play the next note from every starting position and minimize time error.
        for curr_pos in candidates:
            best_prev = None
            min_error = float('inf')
            
            #prev_layer:(position : cost)

            #Someone check my logic but if im ending up in position x regardless I can cut out options which higher previous time error cause the system is memoryless?
            for prev_pos, prev_acc_error in prev_layer.items():
                
                # CALL THE PHYSICS SIMULATION FOR TRAVEL TIME
                dist_cm = abs(curr_pos - prev_pos)
                required_time = get_travel_time(dist_cm)
                
                #this says how long we can sustain
                lost_sustain_cost = required_time
                
                #How many seconds late are we - >
                lateness = max(0.0, required_time- available_time)
                
                punishement_cost = lateness*10
                
                step_error= lost_sustain_cost + punishement_cost
                total_new_error = prev_acc_error + step_error
                
                #Find lowest time error way to end up in position x (cm)
                if total_new_error < min_error:
                    min_error = total_new_error
                    best_prev = prev_pos
            
            #saves as : current_layer_cost{curr_pos1: time error, curr_pos2: time error, ...}
            #           current_layer_parents{curr_pos_end: pos_prev, ...} if you go to position n you came from n-1...
            if best_prev is not None:
                current_layer_cost[curr_pos] = min_error
                current_layer_parents[curr_pos] = best_prev
        
#Not currently useful but can eventually be used as apunishement incase we arrive late, honestly im thinking most error math should be reworked
        if not current_layer_cost:
            # just pick the previous position with lowest error and teleport (accepting huge error)
            best_fallback = min(prev_layer, key=prev_layer.get)
            current_layer_cost = {candidates[0]: prev_layer[best_fallback] + 100} # +100 penalty
            current_layer_parents = {candidates[0]: best_fallback}

        layers.append(current_layer_cost)
        path_traceback.append(current_layer_parents)

    # Backtrack to recontruct the best path
    final_layer = layers[-1]
    
    #path is empty just return nothing
    if not final_layer: return [], []
    
    
    #Position_A: Error=100, Position_B: Error=0.5, Position_C: Error=50 } check for the smallest error
    best_end_pos = min(final_layer, key=final_layer.get)
    optimal_path_cm = [best_end_pos]
    curr = best_end_pos
    
    
    #Path traceback if im at x where did i have to come from? just traces backwards on current_layer_parents
    #Start at index of last note, stop at start of song just kinda a reverse loop to reverse a linked list
    for t in range(len(time_steps_data) - 1, 0, -1):
        if curr in path_traceback[t]:
            #pathtraceback { Current_Pos : Previous_Pos }.
            parent = path_traceback[t][curr]
            optimal_path_cm.append(parent)
            curr = parent
        else:
            # impossible note?
            optimal_path_cm.append(curr)
            
    optimal_path_cm.reverse()
    
    return sorted_times, optimal_path_cm


#AI FUNCTION TO JUST PRINT THE DATA
def print_finger_assignments(times, path_cm, notes):
    """
    Prints which finger (Black/White) hits which note.
    """
    # Create lookup map
    groups = collections.defaultdict(list)
    for n in notes:
        if n.pitch >= SPLIT_POINT:
            groups[n.start].append(n.pitch)
            
    print(f"\n{'TIME':<8} | {'hand':<10} | {'NOTE':<6} | {'TYPE':<6} | {'ASSIGNED FINGER'}")
    print("-" * 65)
    
    for t, hand_cm in zip(times, path_cm):
        pitches = groups[t]
        for p in pitches:
            note_pos = get_note_position_cm(p)
            note_type = "BLACK" if is_black_key(p) else "WHITE"
            
            # Find which finger hit it
            assigned_id = "MISS"
            assigned_type = ""
            
            for finger in ROBOT_FINGERS:
                # Check Type
                if (note_type=="BLACK" and finger['type']=='w') or (note_type=="WHITE" and finger['type']=='b'):
                    continue
                # Check Distance
                finger_loc = hand_cm + finger['offset']
                if abs(note_pos - finger_loc) <= HIT_TOLERANCE_CM:
                    assigned_id = finger['id']
                    assigned_type = "Black" if finger['type'] == 'b' else "White"
                    break
            
            print(f"{t:<8.2f} | {hand_cm:<10.2f} | {p:<6} | {note_type:<6} | Finger {assigned_id} ({assigned_type})")
        
    
    
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import collections

VISUAL_WHITE_KEY_WIDTH = 2.0
VISUAL_BLACK_KEY_WIDTH = 1.2
PIANO_HEADER_HEIGHT = 0.5


# --- PLOTTING ---
def plot_precise_movement_schedule(notes, times, hand_path_cm, robot_config):
    """
    Visualizes the EXACT moment the robot must leave one note to catch the next.
    Splits the path into 'Hold' phases and 'Move' phases.
    Tracks the main carriage center for a clean, readable motor schedule.
    """
    if not times:
        print("No path data to plot.")
        return

    fig, ax = plt.subplots(figsize=(16, 12))
    ax.invert_yaxis()
    
    # Offset visualization so it doesn't overlap the header
    TIME_OFFSET = 0.5 
    
    # Setup Axis
    # 1.5 seconds of extra padding to the bottom so the final note isn't cut off
    max_time = max(times)
    ax.set_ylim(max_time + TIME_OFFSET + 1.5, -0.8) 
    
    ax.set_ylabel("Time (seconds)", fontsize=12)
    ax.set_xlabel("Rail Position (cm)", fontsize=12)
    ax.set_title("Motor Schedule: Center of Hand Movement", fontsize=14)
    
    # --- 1. DRAW PIANO HEADER ---
    draw_piano_header(ax, 0, PIANO_HEADER_HEIGHT)
    
    # Track labels so we don't clutter the legend
    added_labels = set()
    def get_label(name):
        if name not in added_labels:
            added_labels.add(name)
            return name
        return ""

    # --- 2. CALCULATE & DRAW TRAJECTORIES ---
    for i in range(len(times) - 1):
        curr_time = times[i] + TIME_OFFSET
        curr_pos = hand_path_cm[i]
        
        next_time = times[i+1] + TIME_OFFSET
        next_pos = hand_path_cm[i+1]
        
        dist = abs(next_pos - curr_pos)
        travel_duration = get_travel_time(dist)
        
        ideal_departure = next_time - travel_duration
        
        # --- THE REALITY CHECK ---
        if ideal_departure < curr_time:
            actual_departure = curr_time
            actual_arrival = curr_time + travel_duration
            is_late = True
        else:
            actual_departure = ideal_departure
            actual_arrival = next_time 
            is_late = False

        # --- PLOT MAIN CARRIAGE (Center Anchor) ---
        # Segment 1: The Hold
        ax.plot([curr_pos, curr_pos], [curr_time, actual_departure], 
                color='black', linewidth=4, solid_capstyle='round', 
                label=get_label('Carriage Center (Sustain)'))
        
        # Segment 2: The Move
        if dist > 0:
            move_color = 'red' if is_late else '#00aa00'
            style = '--' if is_late else ':'
            
            ax.plot([curr_pos, next_pos], [actual_departure, actual_arrival], 
                    color=move_color, linestyle=style, linewidth=2,
                    label=get_label('Travel (Late)' if is_late else 'Travel (On Time)'))
            
            marker_style = 'x' if is_late else 'v' 
            ax.scatter([curr_pos], [actual_departure], 
                       color='darkorange' if not is_late else 'red', 
                       marker=marker_style, s=80, zorder=10,
                       label=get_label('Departure Trigger'))

            if is_late:
                 ax.scatter([next_pos], [actual_arrival], color='red', marker='o', s=40, zorder=10)

    # --- ADD THE FINAL NOTE HOLD ---
    if len(times) > 0:
        last_time = times[-1] + TIME_OFFSET
        last_pos = hand_path_cm[-1]
        
        # Final Carriage Hold
        ax.plot([last_pos, last_pos], [last_time, last_time + 0.5], 
                color='black', linewidth=4, solid_capstyle='round')

    # --- 3. DRAW NOTES & FINGER HITS (Background Context) ---
    for i, t in enumerate(times):
        c_pos = hand_path_cm[i]
        visual_t = t + TIME_OFFSET
        
        current_notes = [n for n in notes if abs(n.start - t) < 0.02]
        
        for note in current_notes:
            note_pos = get_note_position_cm(note.pitch)
            is_black = is_black_key(note.pitch)
            duration = max(0.1, note.end - note.start)
            
            width = VISUAL_BLACK_KEY_WIDTH if is_black else VISUAL_WHITE_KEY_WIDTH
            rect = patches.Rectangle(
                (note_pos - width/2, visual_t), width, duration, 
                linewidth=1, edgecolor='#555', 
                facecolor='white' if not is_black else '#444', alpha=0.4
            )
            ax.add_patch(rect)
            
            hit_finger_idx = None
            for f_idx, finger in enumerate(robot_config):
                if (is_black and finger['type'] == 'w') or (not is_black and finger['type'] == 'b'): continue
                if abs((c_pos + finger['offset']) - note_pos) <= 0.3:
                    hit_finger_idx = f_idx
                    break
            
            if hit_finger_idx is not None:
                f_offset = robot_config[hit_finger_idx]['offset']
                ax.plot([c_pos + f_offset, note_pos], [visual_t, visual_t], 
                        color='blue', alpha=0.3, linewidth=1)
                ax.scatter([c_pos + f_offset], [visual_t], color='blue', s=10, alpha=0.5)

    ax.grid(True, axis='y', alpha=0.3)
    
    # --- LEGEND UPDATES ---
    # Moved to lower left, increased font size, added a slight background frame for readability over grid lines
    ax.legend(loc='lower left', fontsize=14, framealpha=0.9, edgecolor='#ccc')
    
    plt.tight_layout()
    plt.show()
    

def draw_piano_header(ax, y_pos, height):
    """Draws a realistic piano keyboard along the X-axis at a specific Y-position."""
    min_midi = 21
    max_midi = 71
    
    for pitch in range(min_midi, max_midi + 1):
        if not is_black_key(pitch):
            pos_cm = get_note_position_cm(pitch)
            rect = patches.Rectangle(
                (pos_cm - VISUAL_WHITE_KEY_WIDTH/2, y_pos), 
                VISUAL_WHITE_KEY_WIDTH, height,                 
                linewidth=1, edgecolor='black', facecolor='white', zorder=1
            )
            ax.add_patch(rect)
            
            if (pitch % 12) == 0:
                ax.text(pos_cm, y_pos - height*0.2, f"C{(pitch//12)-1}", 
                        ha='center', va='top', fontsize=8, color='blue', zorder=3)

    for pitch in range(min_midi, max_midi + 1):
        if is_black_key(pitch):
            pos_cm = get_note_position_cm(pitch)
            black_height = height * 0.65
            rect = patches.Rectangle(
                (pos_cm - VISUAL_BLACK_KEY_WIDTH/2, y_pos),
                VISUAL_BLACK_KEY_WIDTH, black_height,
                linewidth=1, edgecolor='black', facecolor='black', zorder=2
            )
            ax.add_patch(rect)
            
def generate_c_command_array(notes, times, hand_path_cm, robot_config):
    """
    Parses the kinematic path and outputs formatted C code 
    containing the sequence of MOVE and PLAY commands.
    """
    commands = []
    current_pos = -1.0
    TIME_OFFSET = 0.5 # Same offset used in plotting
    
    for i in range(len(times)):
        t = times[i]
        target_pos = hand_path_cm[i]
        
        # GENERATE MOVE COMMAND
        if target_pos != current_pos:
            if i == 0:
                # First move to start position
                travel_duration = get_travel_time(target_pos)
                departure = max(0.0, (t + TIME_OFFSET) - travel_duration)
            else:
                # Calculate required departure based on physics
                dist = abs(target_pos - current_pos)
                travel_duration = get_travel_time(dist)
                ideal_departure = (t + TIME_OFFSET) - travel_duration
                actual_departure = max(times[i-1] + TIME_OFFSET, ideal_departure)
                departure = actual_departure
            
            arrival = departure + travel_duration
            
            # Convert cm to mm to preserve 0.5cm increments in an integer
            pos_mm = int(target_pos * 10.0) 
            
            commands.append({
                'action': 'RIGHT_MOVE',
                'val': pos_mm,
                'start': departure,
                'end': arrival
            })
            current_pos = target_pos

       #GENERATE PLAY COMMAND 
        current_notes = [n for n in notes if abs(n.start - t) < 0.02]
        bitmask = 0
        max_end_time = t
        
        for note in current_notes:
            note_pos = get_note_position_cm(note.pitch)
            is_black = is_black_key(note.pitch)
            max_end_time = max(max_end_time, note.end)
            
            # Find the finger id that hits this note
            for finger in robot_config:
                if (is_black and finger['type'] == 'w') or (not is_black and finger['type'] == 'b'):
                    continue
                if abs((target_pos + finger['offset']) - note_pos) <= 0.3:
                    # Set the bit for this specific solenoid (e.g., id 2 -> 0000 0100)
                    bitmask |= (1 << finger['id'])
                    break
        
        if bitmask > 0:
            commands.append({
                'action': 'RIGHT_PLAY',
                'val': bitmask,
                'start': t + TIME_OFFSET,
                'end': max_end_time + TIME_OFFSET
            })


    c_code += "#define RIGHT_MOVE 0\n"
    c_code += "#define RIGHT_PLAY 1\n"
    c_code += "#define LEFT_PLAY  2\n\n"
    
    c_code += "struct command schedule[] = {\n"
    for cmd in commands:

        c_code += f"    {{{cmd['action']}, {cmd['val']}, {cmd['start']:.3f}f, {cmd['end']:.3f}f}},\n"
    c_code += "};\n\n"
    c_code += f"const int SCHEDULE_LENGTH = {len(commands)};\n"
    
    return c_code