#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT11

#define DHT_PIN A6
#define ALARM_PIN 4
#define PWM1_PIN A4
#define PWM2_PIN A5
#define RPM1_PIN 2
#define RPM2_PIN 3

#define ALARM_ON 1
#define ALARM_OFF 0

DHT_Unified dht(DHT_PIN, DHTTYPE);

uint16_t counter_rpm = 0, rpm = 0, rpm_set = 0;
int seconds = 0;
float avgTemp = 0;
unsigned char alarm = ALARM_OFF;
unsigned char alarmRPM = ALARM_OFF;

void setup()
{
  Serial.begin(9600);
  dht.begin();

  pinMode(PWM1_PIN, OUTPUT);
  pinMode(PWM2_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  
  pinMode(DHT_PIN, INPUT);
  digitalWrite(RPM1_PIN, HIGH);
  digitalWrite(RPM2_PIN, HIGH);
  attachInterrupt(0, rpm_fan, FALLING);

  // configure Timer 1 as "CTC mode"
  TCCR1A = TCCR1A & ~(1 << WGM11);
  TCCR1B = TCCR1B | (1 << WGM12);
  TCCR1A = TCCR1A & ~(1 << WGM10);
  
  // set prescaler to 1024
  TCCR1B = TCCR1B & ~(1 << CS11);
  TCCR1B = TCCR1B | (1 << CS12 | 1 << CS10);

  // define output compare value
  OCR1AH = 0x3D;
  OCR1AL = 0x09;

  // enable timer interrupt to 15625
  TIMSK1 = TIMSK1 | (1 << OCIE1A);

  sei();
}

void loop()
{
  delay(1000);
  
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  float temp = event.temperature;
  //float temp = 40;

  Serial.print("Temp: ");
  Serial.println(temp);

  if(seconds == 0)
  {
    avgTemp = temp;
  }

  seconds++;
  
  avgTemp = avgTemp + temp;

  if(seconds >= 10)
  {
    avgTemp = avgTemp / 11;
    
    Serial.print("avgTemp: ");
    Serial.println(avgTemp);
    
    if(avgTemp >= 20)
    {
      rpm_set = avgTemp*8.5-170;

      if(rpm_set < 28)
      {
        rpm_set = 28;
      }
      
      Serial.print("RPM_set: ");
      Serial.print(rpm_set / 2.55);
      Serial.println("%");
      analogWrite(PWM1_PIN, rpm_set);
      analogWrite(PWM2_PIN, rpm_set);
    }
    else if(isnan(avgTemp))
    {
      rpm_set = 255;
      analogWrite(PWM1_PIN, rpm_set);
      analogWrite(PWM2_PIN, rpm_set);
      alarm = ALARM_ON;
    }
    else
    {
      rpm_set = 0;
      analogWrite(PWM1_PIN, rpm_set);
      analogWrite(PWM2_PIN, rpm_set);
    }

    seconds = 0;
    avgTemp = temp;
  }
  else if(isnan(temp))
  {
    rpm_set = 255;
    analogWrite(PWM1_PIN, rpm_set);
    analogWrite(PWM2_PIN, rpm_set);
    alarm = ALARM_ON;
  }
  
  // BEGIN ALARM SECTION

  //alarm = ALARM_ON;
  //alarmRPM = ALARM_ON;

  if(alarm || alarmRPM)
  {
    if(digitalRead(ALARM_PIN))
    {
      digitalWrite(ALARM_PIN, LOW);
    }
    else
    {
      digitalWrite(ALARM_PIN, HIGH);
    }

  }
  alarm = ALARM_OFF;
  
  // END ALARM SECTION
}

void rpm_fan()
{
  counter_rpm++;
}

ISR(TIMER1_COMPA_vect)
{
  cli();

  rpm = counter_rpm * (60 / 2);
  Serial.print("RPM: ");
  Serial.print(rpm/18);
  Serial.print("% ");
  
  // BEGIN RPM CONTROL
  
  alarmRPM = ALARM_OFF;
    
  if(abs((rpm_set / 2.55) - (rpm / 18)) > 10)
  {
    alarmRPM = ALARM_ON;
  }
    
  // END RPM CONTROL

  counter_rpm = 0;

  sei();
}
