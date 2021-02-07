# My TWatch2020 stuff
Based on the code of Dan Geiger (Thanks on the excellent tutorial! https://www.instructables.com/Lilygo-T-Watch-2020-Arduino-Framework/) including Star Trek skin & vibration!

# Issues
* Main problem of this build is the high power consumtion sucking battery empty within an hour. The only way to manage this is to turn off the clock completely after each usage, with a 6 sec press on the button, which makes this quite uncomfortable. At least the time is not lost, because the RTC has an own battery. The Simple Watch has here a much better usability, with a 30 sec timer sending the watch autmatically to a low power mode or starting/stopping the watch by pressing the button. This way the battery holds for a full day. Therefore I've forked also the TWatch_Library in order to modify the Simple Watch to a not so simple watch...  

# License
Licensed under MIT license, because the original stuff was licensed with that, too.##
