#include "MIDIUSB.h"

#include "pins.h"
#include "velocity.h"

bool should_log_to_usb = false;

#define LOG(msg) \
if (should_log_to_usb) { \
  SerialUSB.print(msg); \
}

#define LOGLN(msg) \
if (should_log_to_usb) { \
  SerialUSB.println(msg); \
}

int key_for_kc_fisi(int kc, int fi_or_si);

void sendMidiNoteOn(byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void sendMidiNoteOff(byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

void sendSustainPedalMidi(bool on) {
  // Midi CC 64 on channel 0, >= 64 velocity for "on"
  midiEventPacket_t event = {0x0B, 0xB0, 64, on ? 127 : 0};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

// C3 == keynum 27, midi note 48
// So add 21 to keynum
int midi_note_for_key(int keynum) {
  return keynum + 21;
}

int key_for_kc_fisi(int kc, int fi_or_si) {
  return (fi_or_si * 8) + kc;
}

struct KeySwitchChange {
  int state;

  // TODO: This will overflow every ~70 minutes. Handle it.
  unsigned long micros;
};

KeySwitchChange fi_switch_changes[88];
KeySwitchChange si_switch_changes[88];

// True if the note is currently "on"
// i.e. a midi noteOn message has been sent but no noteOff
bool noteOn[88];

void init_keyswitch_arrays() {
  unsigned long t = micros();
  for (int i = 0; i < 88; ++i) {
    fi_switch_changes[i] = KeySwitchChange {
      state: 1,
      micros: t,
    };
    si_switch_changes[i] = KeySwitchChange {
      state: 1,
      micros: t,
    };
    noteOn[i] = false;
  }
}

void setup() {
  if (should_log_to_usb) {
    while(!SerialUSB);
    SerialUSB.begin(9600);
    SerialUSB.println("Starting!");
  }

  // KC pins are 5, 6, 7, 8, 20, 21, 22, 23
  for (int i = 0; i <= 4; ++i) {
    pinMode(i, INPUT_PULLUP);
  }
  for (int i = 5; i <= 8; ++i) {
    pinMode(i, OUTPUT);
    digitalWrite(i, 1);
  }
  for (int i = 9; i <= 19; ++i) {
    pinMode(i, INPUT_PULLUP);
  }
  // Uncomment here to use pin 13 as LED_BUILTIN
  // pinMode(13, OUTPUT);  // LED_BUILTIN
  for (int i = 20; i <= 23; ++i) {
    pinMode(i, OUTPUT);
    digitalWrite(i, 1);
  }
  for (int i = 24; i <= 29; ++i) {
    pinMode(i, INPUT_PULLUP);
  }

  // Pins for the sustain pedal
  pinMode(30, INPUT_PULLUP);
  pinMode(31, OUTPUT);
  digitalWrite(31, 0);

  init_keyswitch_arrays();
}

int currentSustainStatus = 0;

void loop() {
  int sustainPedalStatus = digitalRead(30);
  if (sustainPedalStatus != currentSustainStatus) {
    LOG("Sustain pedal change: ");
    LOGLN(sustainPedalStatus);
    currentSustainStatus = sustainPedalStatus;
    sendSustainPedalMidi(!sustainPedalStatus);
  }

  for (int kc = 0; kc < 8; ++kc) {
    int pin = kc_pin(kc);
    digitalWrite(pin, 0);

    // Scan the keys
    for (int fisi = 0; fisi <= 10; ++fisi) {
      int keynum = key_for_kc_fisi(kc, fisi);
      unsigned long t = micros();
      int fi_val = digitalRead(fi_pin(fisi));
      if (fi_val != fi_switch_changes[keynum].state) {
          fi_switch_changes[keynum] = KeySwitchChange {
            state: fi_val,
            micros: t,
          };
          
          if (fi_val == 1 && si_switch_changes[keynum].state == 1 && noteOn[keynum]) {
            LOG("noteOff: ");
            LOGLN(midi_note_for_key(keynum));
            noteOn[keynum] = false;
            sendMidiNoteOff(midi_note_for_key(keynum), 127);
          }
      }
      
      int si_val = digitalRead(si_pin(fisi));
      if (si_val != si_switch_changes[keynum].state) {
          si_switch_changes[keynum] = KeySwitchChange {
            state: si_val,
            micros: t,
          };
          if (si_val == 0 && fi_switch_changes[keynum].state == 0 && !noteOn[keynum]) {
            LOG("noteOn: ");
            LOG(midi_note_for_key(keynum));
            LOG(", time since fi: ");
            LOGLN(t - fi_switch_changes[keynum].micros);
            unsigned long hit_time = t - fi_switch_changes[keynum].micros;
            byte velocity = velocity_from_switch_time(hit_time);
            LOG("Velocity: ");
            LOGLN(velocity);
            noteOn[keynum] = true;
            sendMidiNoteOn(midi_note_for_key(keynum), velocity);
          }
      }
    }
    digitalWrite(pin, 1);
  }
}