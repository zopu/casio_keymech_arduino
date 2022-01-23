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
