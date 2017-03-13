# Chord-Organ

##CHORD ORGAN - ARPEGGIATOR with voicing options
Still needs some debugging and a lot of cleanup.

###PROBLEMS
- I'll put this at the top because there are issues.
- Sometimes the button stops responding, mostly in the Arp modes. Wiggle the pots and usually it will start working again.
- Let me know what else messes up. Turning it off and on again will work too.

###USE
Start/Root -- 1v/oct scaled note offset. 

Station/Chord 
- Chord Organ: Pot selects chords.
                   CV jack can either control the chords played or the voicing. More below.
- Arp: Pot selects chords as in Chord-Organ. Same list of chords.
                  CV switches to the clock input in Arp modes.
                  **WARNING** this can be weird when you drop back into the Chord-Organ
                  
Reset/Trig
- Chord Organ: Trigger pretty much when anything happens.
- Arp: Still a trigger output. Pulse is fired every time the left-most note of
                      chord list is played. More on this below.
                      
Out -- Output
  
Cycle through modes by long pressing the reset button. Will only change one mode at a time.
      No status or indication where you are in the list. (Yet)
      Chord-Organ > Arp up > Arp down > Ping pong > Ping pong 2 > random > Chord-Organ


###VOICING
- Easiest to get a feel for it in the Chord Organ but works in Arp mode too. More likely to be buggy in Arp mode.
- Change voicing by holding reset button and turning the Station/Chord pot. The Station/Chord pot will not alter chords again until you turn back to its original position. No indicator lights or anything yet, sorry. The voicing will always change as soon as you turn the pot.
- Change the Station/Chord CV jack to alter the chords or the voicing by holding the reset button and turning the Start/Root pot. Fully left is chord, Middle is voicing, far right isn't used yet. (Will alter both in the future)
- When CV is set to voicing and the combined pot and cv value are larger than the list size, the Chord Organ will wrap and start voicings from the beginning of the list again.
- When CV is set to chords and the combined pot and cv value are larger than the list size, the Chord Organ will stop on the top chord.
- Don't know the prefered way to handle that.
- Once button is released pot will not respond until turned back to previous chord or root position.
- If you accidentally hold the reset button down and don't change anything you will advance to the Arpeggiator. You then need to long press the button 5 more times to get back to the Chord Organ. No idicators yet. This is annoying when it happens. Sorry again.

###ARP MODES
  1. Arp up  -- Reads chord list left to right.
  2. Arp down -- Reads chord list right to left.
  3. Ping pong -- Reads chord left to right then right to left.
  4. Ping pong 2 -- Ping pong but with left-most and right-most note doubled.
  5. Random  -- Reads a random note in the list and plays it.
                   Trigger is fired anytime left-most note is played.

###CONFIGURATION FILE
- New file called CHORDHYP.TXT. There is now a second list for voicings.
- If there is no file the Chord Organ will create it.
- Must have 16 Chord entries before the Voicings or your voicings will be loaded as chords.

###NEXT STEPS - TODO
- Need to fix debouncing of the pots/jacks
- Add "skip notes". Prefix in conf file with letter 's'. This lets us put padding for Chord-Organ but the Arp will skip these notes.
example: [0,4,7,12,s0] - the Arp would only step through 4 notes, 0,4,7,12.
- Add "held notes". Prefix in conf file with letter 'h'. The Arp will hold down this note always.
example: [h0,4,7,12,0] - the Arp would play the root 0 constantly. It would step through 5 notes, 0,4,7,12,0.
- Use both "held" and "skip" notes.
example: [hs0,4,7,11,14] - the Arp would play the root 0 constantly but never step through it. It would step through 4 notes, 4,7,11,14.
