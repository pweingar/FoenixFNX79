From Claude:

Here are a few things worth checking during your hardware testing, roughly in order of how likely they are to show something:

* Startup and LEDs: The WS2812 overflow fix should make the LEDs behave consistently. Try 0xE0/E1/E2 RGB commands after boot and
  confirm they take effect immediately. Also have the host set Caps Lock during the first 1.5 s and check that the LED matches
  once the animation ends.

* Watchdog: Temporarily add while (1); to the main loop and confirm the board resets after about a second. Then hold the PS/2
  clock low with a jumper and check that the keyboard doesn’t hang or reset-loop, and that it recovers when you release the line.

==> while(1) loop triggers reset

* Typematic: Hold a letter, tap Shift and release it, and the letter should keep repeating. Hold ESC and it should now repeat.

==> letter does not keep repeating if I tap shift.
==> If I hold shift and press a key, the uppercase repeats until I releast the shift. Then the lowercase repeats, which seems correct.
==> ESC repeats

* Protocol details: On a logic analyzer, check that a reset sends a single FA then AA, and that scan set 1 extended-key releases
  show E0 (code|0x80). If you can inject a bad-parity byte from the host side, you should see FE come back.
  
==> extended key releases send the E0 prefix followed by the code ored with 0x80.

* Compatibility: Try your usual hosts, in particular any that gave trouble before, since the 0xFE replies to unknown commands are
  the one behavior change a host could notice.

If anything misbehaves, send me what you saw (a logic analyzer capture or a description of the symptom) and which patch you
suspect, and I can dig in. The FIFO work can wait until you’re ready.

