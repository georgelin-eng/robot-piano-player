import pretty_midi
import midi_utils
import collections

"""
    Look ahead N notes 
    We have F fingers on white keys encoded as a random (0, 2, 4, 6) 
        black keys will be encoded as odd numbers (1, 3, ...)
    
    one octave is encoded as (0, )
"""
 
def create_pitch_map():
    C2_PITCH = 36 
    C5_PITCH = 72 + 1
    WHITE_KEY_WIDTH = 2 # cm
    BLACK_KEY_WIDTH = WHITE_KEY_WIDTH / 2 # is placed in centre point between two white keys

    note_range = range(C2_PITCH, C5_PITCH)
    note_names = [pretty_midi.note_number_to_name(n) for n in note_range]
    black_keys = ["#" in name for name in note_names]

    pitch_map = {}
    pitch_map.update({"C2": 0})

    pos = 0
    curr_note = "C2"
    for note_name in note_names[1:-1]:
        prev_note = curr_note
        curr_note = note_name

        if '#' in note_name: # black key
            pos += BLACK_KEY_WIDTH
        else: # white key
            if ('#' in prev_note):
                pos += BLACK_KEY_WIDTH
            else:
                pos += WHITE_KEY_WIDTH

        pitch_map.update({note_name : pos})

    return pitch_map

def pitch_to_position(pitch_name, pitch_map):
    assert not isinstance(pitch_name, int)
    return pitch_map[pitch_name]

#Minimize distance 
def find_best_pos(notes):
    pass
    NOTES_TO_LOOK_AHEAD = 8
    FINGERS = 3
    WHITE_KEY_WIDTH_CM = 2
    
    SPLIT_POINT = 48
    right_hand_notes = []
    left_hand_notes = []
    
    for note in notes:
        if note.pitch >= SPLIT_POINT:
            right_hand_notes.append(note)
        else:
            left_hand_notes.append(note)

    groups = collections.defaultdict(list)
    for note in right_hand_notes:
        groups[note.start].append(note.pitch)
    
    #sort the chords from low to high
    sorted_times = sorted(groups.keys())
    
    time_steps = []
    for time in sorted_times:
        raw_pitches = groups[time]
        
        indices = [midi_to_white_key(pitch) for pitch in raw_pitches] #Convert pitch into linearized scale removing black keys
        indices = sorted([x for x in indices if x is not None]) #Removes black keys
        if indices:
            time_steps.append(indices)
        
    if not time_steps:
        return [], []
    
    #Initializating the logic
    #layers[time] = {valid hand position: min_cost_so_far}
    layers = []
    path_traceback = [] #Stores parents node to eventually reconstruct final path.
    
    
    #assuming cost to start at a position is 0
    first_candidates = get_valid_hand_positions(time_steps[0])
    layers.append({pos: 0 for pos in first_candidates})
    path_traceback.append({})
    
    
    for time in range (1, len(time_steps)):
        current_chord = time_steps[time]
        
        #Get possible positions the hand can be at to play a certain chord 
        candidates = get_valid_hand_positions(current_chord)
        
        current_layer_cost = {}
        current_layer_parents = {}
        
        #Check costs from previous step
        prev_layers = layers[time-1]
        
        #Chord isnt playable just assume hand doesnt move?
        if not candidates:
            layers.append(prev_layers)
            path_traceback.append({k:k for k in prev_layers})
            continue
        
        #Check every position the hand can be in
        for curr_pos in candidates:
            best_prev = None
            min_cost = float('inf')
            
            #Check previous positions we could have come from
            for prev_pos, prev_cost in prev_layers.items():
                
                #Calculate movement cost
                dist_cm = abs(curr_pos - prev_pos) * WHITE_KEY_WIDTH_CM
                total_new_cost = prev_cost + dist_cm
                
                if total_new_cost < min_cost:
                    min_cost = total_new_cost
                    best_prev = prev_pos
            
            if best_prev is not None:
                current_layer_cost[curr_pos] = min_cost
                current_layer_parents[curr_pos] = best_prev
            
        layers.append(current_layer_cost)
        path_traceback.append(current_layer_parents)
        
    final_layer = layers[-1]
    if not final_layer: return [], []            
        
    best_end_pos = min(final_layer, key=final_layer.get)
    optimal_path_indices = [best_end_pos]
    curr = best_end_pos
        
    #rebuilding that best path from the end to the start
    for t in range(len(time_steps) - 1, 0, -1):
        parent = path_traceback[t][curr]
        optimal_path_indices.append(parent)
        curr = parent
        
    #Reverse to get the best path beggining to end
    optimal_path_indices.reverse()    
        
    optimal_path_cm = [idx * WHITE_KEY_WIDTH_CM for idx in optimal_path_indices]
    
    return sorted_times, optimal_path_cm

#functions used to check all posible positions to play a single chord -> this can limit possibilities whenever chords need to be plaed
def get_valid_hand_positions(chord_indices):
    
    FINGER_OFFSETS = [0, 1, 2, 3, 4, 5, 6, 7, 8]
    if not chord_indices:
        return []  

    valid_hs = []
    potential_hs = set()
    for note_index in chord_indices:
        for finger_offset in FINGER_OFFSETS:
    # finger at finger_offset => hand must be at note - finger_offset
            potential_hs.add(note_index - finger_offset)
            
    # Verify which candidates are actually valid
    for h in potential_hs:
        # Where are my fingers physically located if hand is at h?
        finger_positions = {h + off for off in FINGER_OFFSETS}
        
        # Are ALL required notes covered by a finger?
        if all(note in finger_positions for note in chord_indices):
            valid_hs.append(h)
            
    return sorted(list(valid_hs))


# Function to flatten black keys out of the midi number distribution. This linearizes the notes
def midi_to_white_key(midi_pitch):
    pass
    white_key_mapping = {0:0, 2:1, 4:2, 5:3, 7:4, 9:5, 11:6}
    octave = (midi_pitch // 12 ) - 1
    note_in_octave = midi_pitch % 12
    
    if note_in_octave not in white_key_mapping:
        return None #Black key handle seperately
    
    #Absolute index
    return (7*octave) + white_key_mapping[note_in_octave]
    
    
#Function to seperate the notes into chords if multiple are played in same timestamp
def sort_notes_by_timestep(notes):
    pass
    groups_of_notes ={}
    right_hand_notes = []
    left_hand_notes = []
    
    #Seperate LH RH off of a middle pitch
    left_hand_notes, right_hand_notes = midi_utils.seperate_left_right(notes, 48)
    for notes in right_hand_notes:
        if notes.start not in groups_of_notes:
            groups_of_notes[notes.start] = []
        
        groups_of_notes[notes.start].append(notes.pitch)
        
    return sorted(groups_of_notes.keys())

#AI MODIFY LATER JUST FOR TESTING
def get_finger_assignments(times, path_cm, right_hand_notes):
    # 1. Re-group notes so we can look them up by time
    groups = collections.defaultdict(list)
    for n in right_hand_notes:
        groups[n.start].append(n.pitch)
        
    WHITE_KEY_WIDTH_CM = 2.0
    
    print("--- FINGER ASSIGNMENTS ---")
    
    # 2. Walk through the calculated path
    for t, carriage_pos_cm in zip(times, path_cm):
        
        # Convert carriage cm back to an abstract index (e.g., 35.0)
        carriage_index = int(round(carriage_pos_cm / WHITE_KEY_WIDTH_CM))
        
        # Get the notes meant to be played at this time
        pitches = groups[t]
        
        print(f"Time {t:.2f}s | Carriage at {carriage_pos_cm}cm (Index {carriage_index})")
        
        for pitch in pitches:
            # Calculate Note Index (e.g., Middle C is 35)
            note_index = midi_to_white_key(pitch)
            
            # THE MATH: Which finger is touching this note?
            # If Carriage is at 35 and Note is at 37...
            # Finger Offset = 37 - 35 = 2 (The finger at offset 2)
            finger_offset = note_index - carriage_index
            
            print(f"   --> Play Note {pitch} (Index {note_index}) using Finger [{finger_offset}]")
            
def get_path(notes):
    pass

