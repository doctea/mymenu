# mymenu

Still very much a WIP, as are (currently) both of the projects that rely on it.  Get in touch if you've some interest in using or contributing to this.
 
UI providing framework and menu item classes for displaying using the ST7789_t3 library, ILI9341_T3n library, and the Bodmer gfx library (atrophied/probably non-working ss_oled library code too).  With rotary encoder and a couple of buttons for input.  Should be relatively simple to support other libraries/displays by writing a thin wrapper around it (ie write a new DisplayTranslator_XYZLibrary and plumb it into the library appropriately).

Used by the [usb_midi_clocker](https://github.com/doctea/usb_midi_clocker) and [Microlidian](https://github.com/doctea/Microlidian) projects.

See those projects for example of use & extending with new controls.

Uses [midihelpers](https://github.com/doctea/midihelpers) library, which also provides controls for manipulating BPM and clock source settings.

(sidenhancy_version branch is older version that (should) work with [sidenhancy](https://github.com/doctea/sidenhancy) - but that whole project is unfinished and probably coderotting by now)

Also extended by the [parameters](https://github.com/doctea/parameters) library to implement configurable ADC inputs with mapping to configurable parameters.

# Requirements

- (on rp2040 only) [patched bodme/TFT_eSPI library](https://github.com/doctea/TFT_eSPI) needed to fix bug with wrapping not working (leading to missing headers from menus?)
- [Functional-Vlpp](https://registry.platformio.org/libraries/khoih-prog/Functional-Vlpp/installation) library for lambda support

## todo

- Refactor to improve inheritance and reusability of the menu item code?
- Refactor to handle different underlying datatypes better?
- Make work with sidenhancy again
- ~~Make work with the ILI9341 library~~
- ILI9341 library doesn't correctly draw separators; as if the font height doesnt get reported properly, or \n isn't obeyed, or similar?
