# My TWatch2020 stuff
Based on the code of Dan Geiger (Thanks on the excellent tutorial! https://www.instructables.com/Lilygo-T-Watch-2020-Arduino-Framework/) including Star Trek skin & vibration!

# Next steps
* Getting power consumption under control. The original build runs the main loop around 1000 times a second, which seems to me the reason why it sucks the battery empty within an hour. SimpleWatch seems to handle this better and is even able to send the watch to a sleep mode, and recovering it with a <1 sec push on the button, while this build only supports the 2s and 6s pushes enabling and disabling the watch through the power management chip.

# License
Licensed under MIT license, because the original stuff was licensed with that, too.##
