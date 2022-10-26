#include <Arduino.h>
//#include "midi/midi_helpers.h"

String get_note_name(int pitch) {
  if (pitch==-1 || pitch>127) {
    String s = "_"; 
    return s;
  }
  int octave = pitch / 12;
  int chromatic_degree = pitch % 12; 
  const String note_names[] = {
    F("C"), F("C#"), F("D"), F("D#"), F("E"), F("F"), F("F#"), F("G"), F("G#"), F("A"), F("A#"), F("B")
  };
  
  String s = note_names[chromatic_degree] + String(octave);
  return s;
}

const char *get_note_name_c(int pitch) {
  if (pitch==-1 || pitch>127) {
    return "_";
  }
  int octave = pitch / 12;
  int chromatic_degree = pitch % 12;
  const char *note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };
  static char note_name[4];
  sprintf(note_name, "%s%i", note_names[chromatic_degree], octave);
  return note_name;
}