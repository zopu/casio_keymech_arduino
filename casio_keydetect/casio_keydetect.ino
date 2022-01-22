#include "MIDIUSB.h"

int kc_pin(int n);
int fi_pin(int n);
int si_pin(int n);
int key_for_kc_fisi(int kc, int fi_or_si);

void noteOn(byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void noteOff(byte pitch, byte velocity) {
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

struct KeySwitchChange {
  int state;

  // TODO: This will overflow every ~70 minutes. Handle it.
  unsigned long micros;
};

KeySwitchChange fi_switch_changes[88];
KeySwitchChange si_switch_changes[88];

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
  }
}

void setup() {
  while(!SerialUSB);
  SerialUSB.begin(9600);
  SerialUSB.println("Starting!");

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
    SerialUSB.print("Sustain pedal change: ");
    SerialUSB.println(sustainPedalStatus);
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
//          SerialUSB.print("FI key change: ");
//          SerialUSB.print(kc);
//          SerialUSB.print(", ");
//          SerialUSB.print(fisi);
//          SerialUSB.print(", ");
//          SerialUSB.print(keynum);
//          SerialUSB.print(", ");
//          SerialUSB.print(fi_val);
//          SerialUSB.print(", t: ");
//          SerialUSB.println(t);
          fi_switch_changes[keynum] = KeySwitchChange {
            state: fi_val,
            micros: t,
          };

          // C3 == keynum 27, midi note 48
          // So add 21 to keynum
          if (fi_val == 1 && si_switch_changes[keynum].state == 1) {
            SerialUSB.print("noteOff: ");
            SerialUSB.println(keynum + 21);
            noteOff(keynum + 21, 127);
          }
      }
      
      int si_val = digitalRead(si_pin(fisi));
      if (si_val != si_switch_changes[keynum].state) {
//          SerialUSB.print("SI key change: ");
//          SerialUSB.print(kc);
//          SerialUSB.print(", ");
//          SerialUSB.print(fisi);
//          SerialUSB.print(", ");
//          SerialUSB.print(keynum);
//          SerialUSB.print(", ");
//          SerialUSB.print(si_val);
//          SerialUSB.print(", t: ");
//          SerialUSB.println(t);
          si_switch_changes[keynum] = KeySwitchChange {
            state: si_val,
            micros: t,
          };
          if (si_val == 0 && fi_switch_changes[keynum].state == 0) {
            SerialUSB.print("noteOn: ");
            SerialUSB.print(keynum + 21);
            SerialUSB.print(", time since fi: ");
            SerialUSB.println(t - fi_switch_changes[keynum].micros);
            unsigned long hit_time = t - fi_switch_changes[keynum].micros;
            // TODO: / 1800 is picked just to get numbers in roughly the right range
            unsigned long inv_velocity = hit_time / 1800;
            if (inv_velocity > 127) {
              inv_velocity = 127;
            }
            byte velocity = 127 - inv_velocity;
            SerialUSB.print("Velocity: ");
            SerialUSB.println(velocity);
            noteOn(keynum + 21, velocity);
          }
      }
    }
    digitalWrite(pin, 1);
  }
}

int key_for_kc_fisi(int kc, int fi_or_si) {
  return (fi_or_si * 8) + kc;
}

// Defaults to returning pin of KC-0 if not 0<=n<=7
int kc_pin(int n) {
  if (n < 0 || n > 7) {
    return 5;
  }
  if ( n % 2 == 1) {
    return 20 + ((n - 1) / 2);
  } else {
    return 5 + (n / 2);
  }
}

// Defaults to returning pin of FI-0 if not 0<=num<=10
int fi_pin(int n) {
  // FI- pins are 0, 1, 2, 3, 4, 24, 25, 26, 27, 28, 29
  if (n >= 0 && n <= 4) {
    return n;
  }
  if (n >= 5 && n <= 10) {
    return 19 + n;
  }
  return 0;  // FI-0
}

// Defaults to returning pin of SI-0 if not 0<=num<=10
int si_pin(int n) {
  // SI- pins are 15, 16, 17, 18, 19, 9, 10, 11, 12, 13, 14
  if (n >= 0 && n <= 4) {
    return 15 + n;
  }
  if (n >= 5 && n <= 10) {
    return 4 + n;
  }
  return 0;  // FI-0
}
