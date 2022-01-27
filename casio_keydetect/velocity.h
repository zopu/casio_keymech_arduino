// Thanks to https://piecesofpi.co.uk/arduino-midi-keyboard-part-6-velocity-or-is-it-volume/
#define MAX_VEL_CURVE_INDEX 80
const unsigned int concaveCurve[MAX_VEL_CURVE_INDEX+1]={127, 127, 127, 127, 122, 116, 111, 106, 102, 97, 93, 88, 84, 80, 76, 73, 69, 66, 62, 59, 56, 53, 50, 48, 45, 43, 40, 38, 36, 34, 32, 30, 28, 26, 25, 23, 22, 21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 11, 10, 9, 9, 8, 8, 7, 7, 7, 6, 6, 6, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

unsigned int velocity_from_switch_time(unsigned long switch_time_micros) {
    unsigned int t_ms = switch_time_micros / 1000;
    if (t_ms > MAX_VEL_CURVE_INDEX) {
        t_ms = MAX_VEL_CURVE_INDEX;
    }
    return concaveCurve[t_ms];
}