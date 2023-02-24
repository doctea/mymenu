# mymenu

Still very much a WIP, as are (currently) both of the projects that rely on it.  Get in touch if you've some interest in using or contributing to this.
 
UI providing framework and menu item classes for displaying using the ST7789_t3 and Bodmer gfx library (atrophied/probably non-working ss_oled library code too).  With rotary encoder and a couple of buttons for input.  Should be relatively simple to support other libraries/displays by writing a thin wrapper around it.

Used by the [usb_midi_clocker](https://github.com/doctea/usb_midi_clocker) and [Microlidian](https://github.com/doctea/Microlidian) projects.

Uses [midihelpers](https://github.com/doctea/midihelpers) library.

sidenhancy_version branch is older version that (should) work with [sidenhancy](https://github.com/doctea/sidenhancy).  

See those projects for example of use & extension.

Also extended by the [parameters](https://github.com/doctea/parameters) library to implement configurable ADC inputs with mapping to configurable parameters.

## todo

- Refactor to improve inheritance and reusability of the menu item code.
- Refactor to handle different underlying datatypes better.
- Make work with sidenhancy again
- Make work with the ILI9341 library
