# Chord-Organ

##ARPEGGIATOR
Still needs some debugging and a lot of cleanup. 

###USE
- Start/Root -- Pot and CV work as before.

- Station/Chord -- Pot selects chords as in Chord-Organ. Same list of chords.
                  CV switches to the clock input in Arp modes.
                  **WARNING** this can be weird when you drop back into the Chord-Organ
                  
- Reset/Trig -- Still a trigger output. Pulse is fired every time the left-most note of
                      chord list is played. More on this below.
                      
- Out -- Output
  
- Cycle through modes by long pressing the reset button. Will only change one mode at a time.
      No status or indication where you are in the list.
      Chord-Organ > Arp up > Arp down > Ping pong > Ping pong 2 > random > Chord-Organ
  
###ARP MODES
  1. Arp up  -- Reads chord list left to right.
  2. Arp down -- Reads chord list right to left.
  3. Ping pong -- Reads chord left to right then right to left.
  4. Ping pong 2 -- Ping pong but with left-most and right-most note doubled.
                   Trigger is played twice on left-most note.
  5. Random  -- Reads a random note in the list and plays it.
                   Trigger is fired anytime left-most note is played.
