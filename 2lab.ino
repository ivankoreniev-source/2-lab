const int PIN_NTC = A0;
const int PIN_TEMP_OUT = 3;
const int PIN_OR_OUT = 4;

const int redPin = 5;
const int greenPin = 6;
const int bluePin = 9;

bool isDanger = false;
bool isWarning = false;
bool isSafe = false;

void setColor(int redValue, int greenValue, int blueValue)
{
  analogWrite(redPin, redValue);
  analogWrite(greenPin, greenValue);
  analogWrite(bluePin, blueValue);
}

void LED(bool danger, bool warning, bool safe)
{
  if (danger) {
    setColor(255, 0, 0);
    delay(500);
    setColor(0, 0, 0);
    delay(500);
  }
  else if (warning) {
    setColor(255, 165, 0);
    delay(500);
    setColor(0, 0, 0);
    delay(500);
  }
  else if (safe) {
    setColor(255, 255, 255);
    delay(1000);
    setColor(0, 0, 0);
    delay(1000);
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  pinMode(PIN_TEMP_OUT, OUTPUT); // GPIO 3 подає сигнал на вхід OR
  pinMode(PIN_OR_OUT, INPUT);    // GPIO 4 читає сигнал з виходу OR
  pinMode(PIN_NTC, INPUT);

  // задаємо опорну напругу Vref = Vcc
  // АЦП вимірює частку від Vcc (Vout / Vcc)
  // АЦП має 10 біт → 2 ^ 10 = 1024 та -1, оскільки рахуємо від 0
  analogReference(DEFAULT);
}

float ReadNtcTemperature()
{
  const int   ADC_MAX   = 1023;           // 10-бітний АЦП Arduino UNO/Nano (2 ^ 10 - 1 значень)
  const float RSER_OHM  = 4700.0;         // 4.7 кОм до GND
  const float R0_OHM    = 10000.0;        // 10 кОм @ 25°C
  const float BETA      = 3950.0;         // коеф. B для MF52AT (уточнити по даташиту)
  const float T0_K      = 25.0 + 273.15;  // 298.15 K

  int adc = analogRead(PIN_NTC);
  if (adc < 1)
  {
    // захист від ділення на нуль, якщо сенсор від'єднаний
    adc = 1;
  }

  // шукаємо опір терморезистора
  // Rntc = RSER_OHM * ((ADC_MAX / adc) - 1)
  float Rntc = RSER_OHM * ( (float)ADC_MAX / (float)adc - 1.0 );

  // шукаємо залежність температури від опору згідно рівняння термістора
  // https://www.ametherm.com/thermistor/ntc-thermistor-beta
  float invT  = (1.0 / T0_K) + (1.0 / BETA) * log(Rntc / R0_OHM);

  // “інверсія”, щоб повернутись від 1 / T до самої температури T
  float tempK = 1.0 / invT;
  return tempK;
}

void loop()
{
  isSafe    = false;
  isWarning = false;
  isDanger  = false;

  float temp = ReadNtcTemperature() - 273.15;

  digitalWrite(PIN_TEMP_OUT, LOW);
  delayMicroseconds(50);
  bool flameDetected = (digitalRead(PIN_OR_OUT) == HIGH);

  if (temp > 30.0) {
    digitalWrite(PIN_TEMP_OUT, HIGH);
  } else {
    digitalWrite(PIN_TEMP_OUT, LOW);
  }
  delayMicroseconds(50);

  Serial.println(temp);

  if (flameDetected || temp >= 35.0) {
    isDanger = true;
  }
  else if (temp >= 30.0 && temp < 35.0) {
    isWarning = true;
  }
  else {
    isSafe = true;
  }

  LED(isDanger, isWarning, isSafe);
}
