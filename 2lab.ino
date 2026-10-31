const int PIN_NTC = A0;
const int PIN_TEMP_OUT = 3;
const int PIN_OR_OUT = 4;

const int RED_PIN = 5;
const int GREEN_PIN = 6;
const int BLUE_PIN = 9;

const int SERIAL_RATE = 9600;

const int SENSOR_READ_DELAY = 50;
const int DANGER_WARNING_DELAY = 500;
const int SAFE_DELAY = 1000;

const float WARNING_TEMP = 30.0;
const float DANGER_TEMP = 35.0;

void setColor(int redValue, int greenValue, int blueValue)
{
  analogWrite(RED_PIN, redValue);
  analogWrite(GREEN_PIN, greenValue);
  analogWrite(BLUE_PIN, blueValue);
}

void blinkColor(int redValue, int greenValue, int blueValue, int delayMs)
{
  setColor(redValue, greenValue, blueValue);
  delay(delayMs);
  setColor(0, 0, 0);
  delay(delayMs);
}

void DoBlink(float tempC, bool flameDetected)
{
  if (flameDetected || tempC >= DANGER_TEMP) return blinkColor(255, 0, 0, DANGER_WARNING_DELAY);
  if (tempC >= WARNING_TEMP) return blinkColor(255, 165, 0, DANGER_WARNING_DELAY);
  blinkColor(255, 255, 255, SAFE_DELAY);
}

void setup()
{
  Serial.begin(SERIAL_RATE);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

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

float getTemperatureCelsius()
{
  return ReadNtcTemperature() - 273.15;
}

bool checkFlameDetected()
{
  digitalWrite(PIN_TEMP_OUT, LOW);
  delayMicroseconds(SENSOR_READ_DELAY);
  return (digitalRead(PIN_OR_OUT) == HIGH);
}

void updateTempOutSignal(float tempC)
{
  digitalWrite(PIN_TEMP_OUT, tempC > WARNING_TEMP ? HIGH : LOW);
  delayMicroseconds(SENSOR_READ_DELAY);
}

void loop()
{
  float temp = getTemperatureCelsius();
  bool flameDetected = checkFlameDetected();

  updateTempOutSignal(temp);
  Serial.println(temp);
  DoBlink(temp, flameDetected);
}
