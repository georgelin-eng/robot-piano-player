WHITE_KEY_WIDTH_CM = 2.28 # Adjust to your keyboard
RIGHT_WALL_MIDI_PITCH = 77 # The absolute right-most key on your board

# The physical offsets (Positive = Right, Negative = Left)
BLACK_KEY_OFFSETS_CM = {
    1: -(WHITE_KEY_WIDTH_CM / 10),  
    3:  (WHITE_KEY_WIDTH_CM / 10),  
    6: -(WHITE_KEY_WIDTH_CM / 7),   
    8:  0.00,                
    10: (WHITE_KEY_WIDTH_CM / 7)    
}

def calculate_standard_pos(pitch):
    octave = pitch // 12
    note_in_octave = pitch % 12
    octave_offsets = {0: 0.0, 1: 0.5, 2: 1.0, 3: 1.5, 4: 2.0, 5: 3.0, 6: 3.5, 7: 4.0, 8: 4.5, 9: 5.0, 10: 5.5, 11: 6.0}
    
    base_pos = ((octave * 7) + octave_offsets[note_in_octave]) * WHITE_KEY_WIDTH_CM
    
    if note_in_octave in BLACK_KEY_OFFSETS_CM:
        return base_pos + BLACK_KEY_OFFSETS_CM[note_in_octave]
    return base_pos

robot_map = {}
wall_standard_cm = calculate_standard_pos(RIGHT_WALL_MIDI_PITCH)

for pitch in range(53, 78): 
    standard_cm = calculate_standard_pos(pitch)
    # Convert to mirrored robot coordinate (Right Wall = 0.0)
    robot_cm = wall_standard_cm - standard_cm 
    robot_map[pitch] = round(robot_cm, 3)

print("ABSOLUTE_KEY_MAP_CM = {")
for pitch, pos in robot_map.items():
    print(f"    {pitch}: {pos:.3f},")
print("}")