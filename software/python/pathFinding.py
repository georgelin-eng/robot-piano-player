from matplotlib import ticker
import pretty_midi
import midi_utils
import collections
import math

import pretty_midi
import midi_utils
import collections
import math


from calibrationmap import ABSOLUTE_KEY_MAP_CM


HIT_TOLERANCE_CM = 0

WHITE_KEY_WIDTH_CM=2.28

# RH - LF Split
ORIGIN_MIDI_PITCH = 60
SPLIT_POINT = 60

LH_MAX_PITCH = 59

RH_MIN_PITCH = 65    
RH_MAX_PITCH = 89

WHITE_KEY_SOLENOID_SEPERATION_CM = 2.28
BLACK_KEY_WHITE_KEY_SOLENOID_SEPERATION = 1.14
BLACK_KEY_WIDTH = 0.9

# offset is distance in cm from the  left edge)
# 'type': 'w' for White Key Finger, 'b' for Black Key Finger.
ROBOT_FINGERS = [
    {'id': 0, 'offset': 3*WHITE_KEY_SOLENOID_SEPERATION_CM, 'type': 'w'},  
    {'id': 1, 'offset': 2*WHITE_KEY_SOLENOID_SEPERATION_CM + BLACK_KEY_WHITE_KEY_SOLENOID_SEPERATION, 'type': 'b'}, 
    {'id': 2, 'offset': 2*WHITE_KEY_SOLENOID_SEPERATION_CM, 'type': 'w'},   
    {'id': 3, 'offset': 1*WHITE_KEY_SOLENOID_SEPERATION_CM + BLACK_KEY_WHITE_KEY_SOLENOID_SEPERATION, 'type': 'b'},  
    {'id': 4, 'offset': 1*WHITE_KEY_SOLENOID_SEPERATION_CM, 'type': 'w'}
#    {'id': 5, 'offset': BLACK_KEY_WHITE_KEY_SOLENOID_SEPERATION, 'type': 'b'}
#    {'id': 6, 'offset': 0*WHITE_KEY_WIDTH_CM, 'type': 'w'}
]

LEFT_FINGERS = {
    36: 0,  # C2
    37: 1,  # C#2 / Db2
    38: 2,  # D2
    39: 3,  # D#2 / Eb2
    40: 4,  # E2
    41: 5,  # F2
    42: 6,  # F#2 / Gb2
    43: 7,  # G2
    44: 8,  # G#2 / Ab2
    45: 9,  # A2
    46: 10, # A#2 / Bb2
    47: 11  # B2
}



def is_black_key(midi_pitch):
    """Returns True if the MIDI pitch corresponds to a black key."""
    # Black keys: 1(C#), 3(D#), 6(F#), 8(G#), 10(A#)
    return (midi_pitch % 12) in [1, 3, 6, 8, 10]


def get_note_position_cm(midi_pitch):
    """
    Looks up the exact physical robot coordinate from the hardware calibration file.
    """
    return ABSOLUTE_KEY_MAP_CM.get(midi_pitch, 0.0)

"""Basic Physics simulation to estimate time.
dist_cm: Distance hand will need to travel to reach position

Add-ons 
1. Function for acceleration instead of strict time penalty
    a. Eventually max velocity is hit so would an if else statement or a piecewise
2. Initial wind up time for torque to start moving (not sure if thats how the physics works ask George)
""" 
def get_travel_time(dist_cm):

    if dist_cm == 0: return 0.0
    acc_penalty = 0.01
    max_velocity = 60.0  # cm/s
  #  acceleration_penalty = 0.05 # time to accelerate maybe make this into a function ? Just hardcoding a time penalty for time being
    return (dist_cm / max_velocity) + acc_penalty


""" 
Finds all hand positions where the robot can play all the notes in the timestamp.
chord_data: [(note_pos_cm, is_black?)]
"""
    
def get_valid_hand_positions(chord_data):

    if not chord_data: return []
    
    valid_hs = []
    potential_hs = set()
    
    max_finger_offset = max([f['offset'] for f in ROBOT_FINGERS])
    
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
        
        #If the head position is too far then its not a valid hand position
        if(h + max_finger_offset > get_note_position_cm(RH_MIN_PITCH) or h < 0):
           chord_covered = False
           continue
        
        
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
                if abs(note_pos - finger_loc) <= (HIT_TOLERANCE_CM + 0.01):
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
        step_data.sort(key=lambda x: x[0], reverse=True)
       # step_data.sort(key=lambda x: x[0])
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
        
        if not candidates and len(current_chord) > 1:
            salvaged_chord = current_chord.copy()
            
            #keep dropping the lowest note (index 0) until the remaining 
            #notes fit within the physical stretch of the active fingers.
            #This is to prioritize melody
            while not candidates and len(salvaged_chord) > 1:
                salvaged_chord.pop(0) 
                candidates = get_valid_hand_positions(salvaged_chord)
                
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
            
            
"""Solenoid Windup Filter gives a minimum 'actuation time' to action. If action is lower then wanted time,
    will set the action lenght to the minimum and proportionally shift rest of the song."""
def apply_actuation_limits(commands, min_actuation_time_sec):
    accumulated_shift = 0.0
    
    for cmd in commands:
        
        #Add windup from previous command shifts
        cmd['start'] += accumulated_shift
        cmd['end'] += accumulated_shift
        
        duration = cmd['end'] - cmd['start']
        
        if duration < min_actuation_time_sec:
            time_deficit = min_actuation_time_sec - duration
            
            #modify current endtime
            cmd['end'] += time_deficit
            
            accumulated_shift += time_deficit
            
    return commands
            
def generate_c_command_array(left_notes, right_notes, right_times, right_path_cm, right_config, left_config):
    
    initial_setup_mm = round(float(right_path_cm[0] * 10.0),3) if right_path_cm else 0
    #  BUILD THE MASTER TIMELINE
    # Combine all notes and extract every unique start time in the whole song
    all_notes = left_notes + right_notes
    #master_times = sorted(list(set([n.start for n in all_notes])))
    master_times = sorted(list(set([round(n.start, 3) for n in all_notes])))
    
    # dictionary to instantly look up where the right hand should be at any given right-hand note time
    right_pos_map = {round(t, 3): pos for t, pos in zip(right_times, right_path_cm)}
    
    commands = []
    current_pos = -1.0 # Unknown starting position
    TIME_OFFSET = 0.5 
    
    for t in master_times:
        rounded_t = round(t, 3)
        
        # RIGHT HAND MOVEMENT 
        # Did the pathfinding algorithm say the right hand plays a note here?
        if rounded_t in right_pos_map:
            target_pos = right_pos_map[rounded_t]
            
            # Since current_pos starts at the setup location, this will naturally 
            # ignore the first note (because it's already there) and only calculate 
            # commands for the moves that happen mid-song.
            if target_pos != current_pos:
                dist = abs(target_pos - current_pos)
                travel_duration = get_travel_time(dist)
                
                ideal_departure = (t + TIME_OFFSET) - travel_duration
                actual_departure = max(0.0, ideal_departure) # Safe to use 0.0 floor again
                
                commands.append({
                    'action': 'MOVE',
                    'solenoid_or_position': round(float(target_pos * 10.0),3), 
                    'start': actual_departure,
                    'end': actual_departure + travel_duration
                })
                current_pos = target_pos
                
        # UNIFIED PLAY COMMAND 
        # Find every note (left or right) that strikes on this exact millisecond
        #current_active_notes = [n for n in all_notes if abs(n.start - t) < 0.02]
        current_active_notes = [n for n in all_notes if round(n.start, 3) == t]
        bitmask = 0
        max_end_time = t
        
        for note in current_active_notes:
            max_end_time = max(max_end_time, note.end)
            
            if note.pitch <= LH_MAX_PITCH:
                # LEFT HAND LOGIC (Bits 0-11) 
                if note.pitch in left_config:
                    finger_id = left_config[note.pitch]
                    bitmask |= (1 << finger_id)
            else:
                #RIGHT HAND LOGIC (Bits 12-19)
                note_pos = get_note_position_cm(note.pitch)
                is_black = is_black_key(note.pitch)
                
                for finger in right_config:
                    if (is_black and finger['type'] == 'w') or (not is_black and finger['type'] == 'b'):
                        continue
                    
                    # Does this finger line up with the note from our current position?
                    if abs((current_pos + finger['offset']) - note_pos) <= 0.3:
                        bitmask |= (1 << (finger['id'] + 12)) # Shift up by 12
                        break
        
        # If at least one finger was triggered, append the play command
        if bitmask > 0:
            commands.append({
                'action': 'PLAY',
                'solenoid_or_position': bitmask,
                'start': t + TIME_OFFSET,
                'end': max_end_time + TIME_OFFSET
            })

    commands.sort(key=lambda x: x['start'])
    
    # FILTER FOR MINIMUM ACTUATION TIME
    commands = apply_actuation_limits(commands, min_actuation_time_sec=0.1)
    
    c_code = "#define MOVE 0\n"
    c_code += "#define PLAY 1\n\n"
    
    c_code += f"const int INITIAL_MOTOR_POSITION_MM = {initial_setup_mm};\n\n"
    c_code += "struct command {\n"
    c_code += "    uint8_t action;\n"
    c_code += "    uint32_t solenoid_or_position;\n"
    c_code += "    float start_time;\n"
    c_code += "    float end_time;\n"
    c_code += "};\n\n"
    
    c_code += "struct command schedule[] = {\n"
    for cmd in commands:
        c_code += f"    {{{cmd['action']}, {cmd['solenoid_or_position']}, {cmd['start']:.3f}f, {cmd['end']:.3f}f}},\n"
    c_code += "};\n\n"
    c_code += f"const int SCHEDULE_LENGTH = {len(commands)};\n"
    
    return c_code



#AI PLOTTING
import matplotlib.pyplot as plt
import matplotlib.patches as patches

# --- VISUALIZATION CONSTANTS ---
VISUAL_BLACK_KEY_WIDTH = 1.2
PIANO_HEADER_HEIGHT = 2
TIME_OFFSET = 2.5 

def plot_robot_performance(all_notes, rh_times, rh_path_cm, right_config, print_plot):
    """
    Plots the master timeline of the song, the static left hand, 
    and the physical kinematic trajectory of the mobile right hand.
    """
    
    max_t = max([n.end for n in all_notes] + (rh_times if rh_times else [0]))
    
    dynamic_height = max(12.0, max_t / 2.5)
    
    fig, ax = plt.subplots(figsize=(16, dynamic_height))
    ax.invert_yaxis()
    ax.invert_xaxis()
    
    # 1. Setup Axes (Dynamically size based on the song length)
    ax.set_ylim(max_t + TIME_OFFSET + 1.0, -0.8)
    ax.set_xlabel("Physical Position relative to C3 (cm)", fontsize=12)
    ax.set_ylabel("Time (seconds)", fontsize=12)
    ax.set_title("Robot Performance: Left Hand (Static) & Right Hand (Mobile)", fontsize=16, pad=20)

    # 2. Draw Piano Header (From C2 to F6)
    header_y = 0
    # Draw White Keys First
    for p in range(24, 78): 
        pos = get_note_position_cm(p)
        if not is_black_key(p):
            ax.add_patch(patches.Rectangle((pos - WHITE_KEY_WIDTH_CM/2, header_y), WHITE_KEY_WIDTH_CM, PIANO_HEADER_HEIGHT, 
                                           ec='black', fc='#f0f0f0', zorder=1))
            #if p % 12 == 0:
            #    ax.text(pos, header_y - 0.1, f"C{(p//12)-1}", ha='center', va='top', fontsize=9, color='blue')
                
    # Draw Black Keys Second (So they overlay)
    for p in range(24, 78):
        if is_black_key(p):
            pos = get_note_position_cm(p)
            ax.add_patch(patches.Rectangle((pos - VISUAL_BLACK_KEY_WIDTH/2, header_y), VISUAL_BLACK_KEY_WIDTH, PIANO_HEADER_HEIGHT*0.65, 
                                           ec='black', fc='#333333', zorder=2))

    # 3. Draw the Song Notes (The Background "Synthesia" falling blocks)
    for n in all_notes:
        pos = get_note_position_cm(n.pitch - 12)
        black_key = is_black_key(n.pitch)
        w = VISUAL_BLACK_KEY_WIDTH if black_key else WHITE_KEY_WIDTH_CM
        
        # Color Right Hand notes Grey/Dark Grey
        color = '#888888' if black_key else '#dddddd'
        
        # Color Left Hand notes Light Blue/Dark Blue
        if n.pitch < ORIGIN_MIDI_PITCH:
            color = '#1f4e79' if black_key else '#5a9bd5'
            pos = get_note_position_cm(n.pitch - 24)  

        ax.add_patch(patches.Rectangle((pos - w/2, n.start + TIME_OFFSET), w, max(0.05, n.end - n.start), 
                                       ec='#999', fc=color, alpha=0.5, zorder=0))

    # 4. Draw Right Hand Motor Trajectory (The solid black line)
    if rh_times and rh_path_cm:
        added_labels = set()
        def label(name):
            if name not in added_labels: added_labels.add(name); return name
            return ""

        for i in range(len(rh_times) - 1):
            curr_t, curr_p = rh_times[i] + TIME_OFFSET, rh_path_cm[i] + 7 * WHITE_KEY_WIDTH_CM
            next_t, next_p = rh_times[i+1] + TIME_OFFSET, rh_path_cm[i+1] + 7 * WHITE_KEY_WIDTH_CM
            
            # Use your custom physics simulation here
            travel = get_travel_time(abs(next_p - curr_p))
            ideal_dep = next_t - travel
            
            # Reality check: Are we late?
            late = ideal_dep < curr_t
            dep_t = curr_t if late else ideal_dep
            arr_t = curr_t + travel if late else next_t

            # Draw Hold
            ax.plot([curr_p, curr_p], [curr_t, dep_t], color='black', lw=4, solid_capstyle='round', label=label('Motor Sustain'))
            
            # Draw Move
            if abs(next_p - curr_p) > 0:
                ax.plot([curr_p, next_p], [dep_t, arr_t], color='red' if late else '#00aa00', ls='--' if late else ':', lw=2, label=label('Motor Travel'))
                ax.scatter([curr_p], [dep_t], color='darkorange' if not late else 'red', marker='v' if not late else 'x', s=80, zorder=10, label=label('Departure Trigger'))

            # Draw Finger Strikes for this timestamp
            # active_notes = [n for n in all_notes if abs(n.start - (curr_t - TIME_OFFSET)) < 0.02 and n.pitch >= ORIGIN_MIDI_PITCH]
            # for n in active_notes:
            #     n_pos = get_note_position_cm(n.pitch)
            #     for f in right_config:
            #         if (is_black_key(n.pitch) and f['type'] == 'w') or (not is_black_key(n.pitch) and f['type'] == 'b'): continue
            #         if abs((curr_p + f['offset']) - n_pos) <= HIT_TOLERANCE_CM:
            #             ax.plot([curr_p + f['offset'], n_pos], [curr_t, curr_t], color='red', alpha=0.5, lw=2)
            #             ax.scatter([curr_p + f['offset']], [curr_t], color='red', s=30, zorder=5)
            #             break

        # Final Hold
        last_t, last_p = rh_times[-1] + TIME_OFFSET, rh_path_cm[-1] + 7 * WHITE_KEY_WIDTH_CM
        ax.plot([last_p, last_p], [last_t, last_t + 0.5], color='black', lw=4, solid_capstyle='round')

    # 5. Draw Dynamic Deadzones and Hand Zones
    
    # --- Middle Deadzone (The Gap Between Hands) ---
    # Because our axis is mirrored (higher cm = further left), 
    # we ADD width to get the left edge, and SUBTRACT width to get the right edge.
    rh_left_edge = get_note_position_cm(RH_MIN_PITCH - 12) + (WHITE_KEY_WIDTH_CM / 2)
    lh_right_edge = get_note_position_cm(LH_MAX_PITCH - 19) - (WHITE_KEY_WIDTH_CM / 2)
    
    ax.axvspan(lh_right_edge, rh_left_edge, color='red', alpha=0.1, hatch='//', zorder=0)
    
    mid_deadzone_center = (rh_left_edge + lh_right_edge) / 2
    ax.text(mid_deadzone_center, -1, "DEADZONE (SPLIT)", color='red', alpha=0.6, fontsize=12, weight='bold', ha='center')


    # --- Dynamic Text Labels for Active Zones ---
    # Pushed 10cm deep into their respective zones so they don't overlap the red boxes
    ax.text(lh_right_edge + 10.0, -1, "LEFT HAND ZONE", color='blue', alpha=0.5, fontsize=14, weight='bold', ha='center')
    ax.text(rh_left_edge - 10.0, -1, "RIGHT HAND ZONE", color='black', alpha=0.5, fontsize=14, weight='bold', ha='center')
    
    # Dynamic text placement based on your origin
    #ax.text(origin_pos + 12.0, 0.2, "LEFT HAND ZONE", color='blue', alpha=0.5, fontsize=14, weight='bold', ha='center')
   # ax.text(origin_pos - 12.0, 0.2, "RIGHT HAND ZONE", color='black', alpha=0.5, fontsize=14, weight='bold', ha='center')

    ax.grid(True, axis='y', alpha=0.3)
    ax.legend(loc='lower left', fontsize=12, framealpha=0.9)
    plt.tight_layout()
    
    if(print_plot == 1):
        plt.show()
    else: 
        return fig

def plot_piano(print_plot):
    """
    Plots the master timeline of the song, the static left hand, 
    and the physical kinematic trajectory of the mobile right hand.
    """
    
    
    fig, ax = plt.subplots(figsize=(16, 5))
    ax.invert_yaxis()
    ax.invert_xaxis()
    
    # 1. Setup Axes (Dynamically size based on the song length)
    ax.set_ylim(TIME_OFFSET, -0.4)
    ax.set_xlim(90, 10)

    ax.set_xlabel("", fontsize=12)
    ax.set_ylabel("", fontsize=12)
    ax.set_yticks([])
    ax.set_xticks([])
    #ax.set_title("Robot Performance: Left Hand (Static) & Right Hand (Mobile)", fontsize=16, pad=20)

    # 2. Draw Piano Header (From C2 to F6)
    header_y = 0
    # Draw White Keys First
    for p in range(24, 78): 
        pos = get_note_position_cm(p)
        if not is_black_key(p):
            ax.add_patch(patches.Rectangle((pos - WHITE_KEY_WIDTH_CM/2, header_y), WHITE_KEY_WIDTH_CM, PIANO_HEADER_HEIGHT, 
                                           ec='black', fc='#f0f0f0', zorder=1))

    # Draw Black Keys Second (So they overlay)
    for p in range(24, 78):
        if is_black_key(p):
            pos = get_note_position_cm(p)
            ax.add_patch(patches.Rectangle((pos - VISUAL_BLACK_KEY_WIDTH/2, header_y), VISUAL_BLACK_KEY_WIDTH, PIANO_HEADER_HEIGHT*0.65, 
                                           ec='black', fc='#333333', zorder=2))
    # 5. Draw Dynamic Deadzones and Hand Zones
    
    # --- Middle Deadzone (The Gap Between Hands) ---
    # Because our axis is mirrored (higher cm = further left), 
    # we ADD width to get the left edge, and SUBTRACT width to get the right edge.
    rh_left_edge = get_note_position_cm(RH_MIN_PITCH - 12) + (WHITE_KEY_WIDTH_CM / 2)
    lh_right_edge = get_note_position_cm(LH_MAX_PITCH - 19) - (WHITE_KEY_WIDTH_CM / 2)
    
    ax.axvspan(lh_right_edge, rh_left_edge, color='red', alpha=0.1, hatch='//', zorder=0)
    
    mid_deadzone_center = (rh_left_edge + lh_right_edge) / 2
    ax.text(mid_deadzone_center, -.5, "DEADZONE (SPLIT)", color='red', alpha=0.6, fontsize=12, weight='bold', ha='center')


    # --- Dynamic Text Labels for Active Zones ---
    # Pushed 10cm deep into their respective zones so they don't overlap the red boxes
    ax.text(lh_right_edge + 10.0, -.5, "LEFT HAND ZONE", color='blue', alpha=0.5, fontsize=14, weight='bold', ha='center')
    ax.text(rh_left_edge - 10.0, -.5, "RIGHT HAND ZONE", color='black', alpha=0.5, fontsize=14, weight='bold', ha='center')
    

    ax.grid(True, axis='y', alpha=0.3)
    ax.legend(loc='lower left', fontsize=12, framealpha=0.9)
    plt.tight_layout()
    
    if(print_plot == 1):
        plt.show()
    else: 
        return fig

def plot_robot_movement(all_notes, rh_times, rh_path_cm, right_config, print_plot):
    """
    Plots the master timeline of the song, the static left hand, 
    and the physical kinematic trajectory of the mobile right hand.
    """
    
    max_t = max([n.end for n in all_notes] + (rh_times if rh_times else [0]))
    
    dynamic_height = max(12.0, max_t / 2.5)
    
    fig, ax = plt.subplots(figsize=(16, dynamic_height))
    ax.invert_yaxis()
    ax.invert_xaxis()
    
    # 1. Setup Axes (Dynamically size based on the song length)
    ax.set_ylim(max_t + 1.0, -0.8)
    ax.set_xlim(90, 10)
    ax.set_xlabel("Physical Position (cm)", fontsize=12)
    ax.set_ylabel("Time (seconds)", fontsize=12)
    ax.yaxis.set_major_locator(ticker.MultipleLocator(5))
    ax.yaxis.set_minor_locator(ticker.MultipleLocator(5))


    # 2. Draw Piano Header (From C2 to F6)
    header_y = 0
    # Draw White Keys First
    for p in range(24, 78): 
        pos = get_note_position_cm(p)
        if not is_black_key(p):
            ax.add_patch(patches.Rectangle((pos - WHITE_KEY_WIDTH_CM/2, header_y), WHITE_KEY_WIDTH_CM, 0, 
                                           ec='white', fc='#f0f0f0', zorder=0))
            #if p % 12 == 0:
            #    ax.text(pos, header_y - 0.1, f"C{(p//12)-1}", ha='center', va='top', fontsize=9, color='blue')
                
    # Draw Black Keys Second (So they overlay)
    for p in range(24, 78):
        if is_black_key(p):
            pos = get_note_position_cm(p)
            ax.add_patch(patches.Rectangle((pos - VISUAL_BLACK_KEY_WIDTH/2, header_y), VISUAL_BLACK_KEY_WIDTH, 0, 
                                           ec='white', fc='#333333', zorder=0))

    last_time = 0
    # 3. Draw the Song Notes (The Background "Synthesia" falling blocks)
    for n in all_notes:
        pos = get_note_position_cm(n.pitch - 12)
        black_key = is_black_key(n.pitch)
        w = VISUAL_BLACK_KEY_WIDTH if black_key else WHITE_KEY_WIDTH_CM
        
        # Color Right Hand notes Grey/Dark Grey
        color = '#888888' if black_key else '#dddddd'
        
        # Color Left Hand notes Light Blue/Dark Blue
        if n.pitch < ORIGIN_MIDI_PITCH:
            color = '#1f4e79' if black_key else '#5a9bd5'
            pos = get_note_position_cm(n.pitch - 24)  

        ax.add_patch(patches.Rectangle((pos - w/2, n.start), w, max(0.05, n.end - n.start), 
                                       ec='#999', fc=color, alpha=0.5, zorder=0))
        last_time = n.end       


    # 4. Draw Right Hand Motor Trajectory (The solid black line)
    if rh_times and rh_path_cm:
        added_labels = set()
        def label(name):
            if name not in added_labels: added_labels.add(name); return name
            return ""

        for i in range(len(rh_times) - 1):
            curr_t, curr_p = rh_times[i] , rh_path_cm[i] + 7 * WHITE_KEY_WIDTH_CM
            next_t, next_p = rh_times[i+1] , rh_path_cm[i+1] + 7 * WHITE_KEY_WIDTH_CM
            
            # Use your custom physics simulation here
            travel = get_travel_time(abs(next_p - curr_p))
            ideal_dep = next_t - travel
            
            # Reality check: Are we late?
            late = ideal_dep < curr_t
            dep_t = curr_t if late else ideal_dep
            arr_t = curr_t + travel if late else next_t

            # Draw Hold
            ax.plot([curr_p, curr_p], [curr_t, dep_t], color='black', lw=4, solid_capstyle='round', label=label('Motor Sustain'))
            
            # Draw Move
            if abs(next_p - curr_p) > 0:
                ax.plot([curr_p, next_p], [dep_t, arr_t], color='red' if late else '#00aa00', ls='--' if late else ':', lw=2, label=label('Motor Travel'))
                ax.scatter([curr_p], [dep_t], color='darkorange' if not late else 'red', marker='v' if not late else 'x', s=80, zorder=10, label=label('Departure Trigger'))


        # Final Hold
        last_t, last_p = rh_times[-1] , rh_path_cm[-1] + 7 * WHITE_KEY_WIDTH_CM
        ax.plot([last_p, last_p], [last_t, last_time], color='black', lw=4, solid_capstyle='round')

    # 5. Draw Dynamic Deadzones and Hand Zones
    
    # --- Middle Deadzone (The Gap Between Hands) ---
    # Because our axis is mirrored (higher cm = further left), 
    # we ADD width to get the left edge, and SUBTRACT width to get the right edge.
    rh_left_edge = get_note_position_cm(RH_MIN_PITCH - 12) + (WHITE_KEY_WIDTH_CM / 2)
    lh_right_edge = get_note_position_cm(LH_MAX_PITCH - 19) - (WHITE_KEY_WIDTH_CM / 2)
    
    ax.axvspan(lh_right_edge, rh_left_edge, color='red', alpha=0.1, hatch='//', zorder=0)

    ax.grid(True, axis='y', alpha=0.3)
    ax.legend(loc='lower left', fontsize=12, framealpha=0.9)
    plt.tight_layout()
    
    if(print_plot == 1):
        plt.show()
    else: 
        return fig

import os

def compile_cnc_schedule(midi_filepath, right_hand_config):
    # --- 1. SYNC THE MATH ---
    # Force Pathfinder to obey the GUI finger config
    global ROBOT_FINGERS
    ROBOT_FINGERS = right_hand_config

    print(f"GUI selected file: {midi_filepath}")
    
    # --- 2. OS PATH FILTERING & TELEPORTATION ---
    base_name = os.path.basename(midi_filepath)
    song_name, ext = os.path.splitext(base_name)
    
    # Teleport Python's working directory so midi_utils can find the config/ folder
    target_dir = os.path.dirname(os.path.dirname(midi_filepath))
    os.chdir(target_dir)
    print(f"Re-aligned working directory to: {target_dir}")
    
    # --- 3. EXTRACT & PREVIEW ---
    # Extract ALL notes using your untouched utility
    all_notes = midi_utils.midi_to_notes(song_name) 
    #all_notes = midi_utils.fit_song_into_keyboard(all_notes)
    
    # Save the trimmed preview file
    #midi_utils.notes_to_midi(all_notes, filename=f"songs/{song_name}_preview.mid")
    #print(f"Saved trimmed preview to songs/{song_name}_preview.mid")
    
    # --- 4. STRICT PRE-FILTERING ---
    left_hand_notes = [n for n in all_notes if n.pitch <= LH_MAX_PITCH]
    right_hand_notes = [n for n in all_notes if RH_MIN_PITCH <= n.pitch <= RH_MAX_PITCH]
    
    print(f"Found {len(left_hand_notes)} bass notes and {len(right_hand_notes)} melody notes.")
    
    # --- 5. PATHFINDING & COMPILATION ---
    print("Running pathfinding algorithm on Right Hand...")
    rh_times, rh_path_cm = find_best_time_path(right_hand_notes)
    
    if not rh_times:
        print("WARNING: No valid path found! The chord might be physically impossible to span.")
    else:
        print("Path found. Generating C code...")
        c_code = generate_c_command_array(
            left_notes=left_hand_notes, 
            right_notes=right_hand_notes, 
            right_times=rh_times, 
            right_path_cm=rh_path_cm, 
            right_config=ROBOT_FINGERS, 
            left_config=LEFT_FINGERS
        )
        
        fig = plot_robot_movement(
            all_notes=all_notes, 
            rh_times=rh_times, 
            rh_path_cm=rh_path_cm, 
            right_config=ROBOT_FINGERS,
            print_plot = 0
        )
        
        out_path = "robot_schedule.h"
        with open(out_path, "w") as f:
            f.write(c_code)
            
        print(f"✅ Successfully saved C array to '{out_path}'!")
        
        return c_code, fig