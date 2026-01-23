#include <LiquidCrystal.h>
#include <Servo.h>

// Inisialisasi LCD (RS, Enable, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Inisialisasi Servo Motor
Servo servoMotor;

// Pin Definitions
const int trigPin = 9;        // Trigger pin sensor ultrasonik
const int echoPin = 8;        // Echo pin sensor ultrasonik
const int irSensorPin = 7;    // Pin sensor infrared
const int pushButton = 6;     // Pin pushbutton (mode manual)
const int sensorToggleBtn = A3; // Pin pushbutton (on/off sensor)
const int servoPin = 10;      // Pin servo motor
const int ledGreen = A0;      // LED hijau (jalan)
const int ledRed = A1;        // LED merah (stop)
const int buzzer = A2;        // Buzzer
const int sensorLED = A4;     // LED indikator sensor aktif

// Variabel Global
long duration;
int distance;
bool gateOpen = false;
bool manualMode = false;
bool sensorsEnabled = true;   // Status sensor aktif/tidak
int vehicleCount = 0;
unsigned long lastDetectionTime = 0;
unsigned long lastSensorToggleTime = 0;
const unsigned long gateCloseDelay = 3000; // 3 detik delay sebelum tutup otomatis
const unsigned long buttonDebounceDelay = 300; // Debounce delay

void setup() {
  // Inisialisasi Serial Monitor
  Serial.begin(9600);
  
  // Setup LCD
  lcd.begin(16, 2);
  lcd.print(" SELAMAT DATANG ");
  lcd.setCursor(0, 1);
  delay(2000);
  
  // Setup Pin Modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(irSensorPin, INPUT);
  pinMode(pushButton, INPUT_PULLUP);
  pinMode(sensorToggleBtn, INPUT_PULLUP);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(sensorLED, OUTPUT);
  
  // Setup Servo
  servoMotor.attach(servoPin);
  servoMotor.write(0); // Posisi tutup (0 derajat)
  
  // Setup awal
  digitalWrite(ledRed, HIGH);   // LED merah menyala (gate tutup)
  digitalWrite(ledGreen, LOW);  // LED hijau mati
  digitalWrite(sensorLED, HIGH); // LED sensor aktif menyala
  
  // Tampilan awal LCD
  updateLCD();
  
}

void loop() {
  // Baca sensor ultrasonik (hanya jika sensor aktif)
  if (sensorsEnabled) {
    distance = readUltrasonicSensor();
  } else {
    distance = 999; // Nilai default saat sensor mati
  }
  
  // Baca sensor infrared (hanya jika sensor aktif)
  bool irDetected = false;
  if (sensorsEnabled) {
    irDetected = !digitalRead(irSensorPin); // Assuming active LOW
  }
  
  // Baca pushbutton mode manual
  bool buttonPressed = !digitalRead(pushButton);
  
  // Baca pushbutton toggle sensor
  bool sensorTogglePressed = !digitalRead(sensorToggleBtn);
  
  // Handle toggle sensor dengan debouncing
  if (sensorTogglePressed && (millis() - lastSensorToggleTime > buttonDebounceDelay)) {
    toggleSensors();
    lastSensorToggleTime = millis();
  }
  
  // Mode manual dengan pushbutton
  if (buttonPressed) {
    delay(200); // Debouncing
    manualMode = !manualMode;
    
    if (manualMode) {
      lcd.clear();
      lcd.print("MODE MANUAL");
      lcd.setCursor(0, 1);
      lcd.print("Tekan untuk buka");
      delay(1000);
    }
    
    if (manualMode && gateOpen) {
      closeGate();
    } else if (manualMode && !gateOpen) {
      openGate();
    }
    
    if (!manualMode) {
      updateLCD();
    }
  }
  
  // Mode otomatis (hanya jika sensor aktif)
  if (!manualMode && sensorsEnabled) {
    // Deteksi kendaraan (sensor ultrasonik < 20cm ATAU sensor IR aktif)
    if ((distance > 0 && distance < 20) || irDetected) {
      if (!gateOpen) {
        vehicleDetected();
        openGate();
        vehicleCount++;
      }
      lastDetectionTime = millis();
    }
    
    // Tutup palang otomatis setelah delay
    if (gateOpen && (millis() - lastDetectionTime > gateCloseDelay)) {
      if (distance >= 20 && !irDetected) {
        closeGate();
      }
    }
  }
  
  // Update LCD jika tidak dalam mode manual
  if (!manualMode) {
    updateLCD();
  }
  
  // Debug ke Serial Monitor
  Serial.print("Sensors: ");
  Serial.print(sensorsEnabled ? "ON" : "OFF");
  Serial.print(" | Distance: ");
  Serial.print(distance);
  Serial.print(" cm | IR: ");
  Serial.print(irDetected ? "Detected" : "Clear");
  Serial.print(" | Gate: ");
  Serial.println(gateOpen ? "Open" : "Closed");
  
  delay(100);
}

int readUltrasonicSensor() {
  // Trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Baca echo
  duration = pulseIn(echoPin, HIGH);
  
  // Hitung jarak (cm)
  int dist = duration * 0.034 / 2;
  
  // Filter pembacaan yang tidak valid
  if (dist > 400 || dist < 2) {
    return 999; // Nilai error
  }
  
  return dist;
}

void openGate() {
  if (!gateOpen) {
    lcd.clear();
    lcd.print("MEMBUKA PALANG");
    lcd.setCursor(0, 1);
    lcd.print("Silakan Masuk..");
    
    // Animasi buka palang (0 ke 90 derajat)
    for (int pos = 0; pos <= 90; pos += 5) {
      servoMotor.write(pos);
      delay(50);
    }
    
    gateOpen = true;
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, HIGH);
    
    // Bunyi buzzer konfirmasi
    tone(buzzer, 1000, 300);
    delay(400);
    tone(buzzer, 1200, 300);
    
    Serial.println("Palang dibuka!");
  }
}

void closeGate() {
  if (gateOpen) {
    lcd.clear();
    lcd.print("MENUTUP PALANG");
    lcd.setCursor(0, 1);
    lcd.print("Harap Tunggu...");
    
    // Animasi tutup palang (90 ke 0 derajat)
    for (int pos = 90; pos >= 0; pos -= 5) {
      servoMotor.write(pos);
      delay(50);
    }
    
    gateOpen = false;
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledRed, HIGH);
    
    // Bunyi buzzer konfirmasi
    tone(buzzer, 800, 500);
    
    Serial.println("Palang ditutup!");
  }
}

void vehicleDetected() {
  // Efek suara deteksi kendaraan
  tone(buzzer, 2000, 100);
  delay(150);
  tone(buzzer, 2000, 100);
  
  Serial.println("Kendaraan terdeteksi!");
}

void updateLCD() {
  lcd.clear();
  
  // Baris pertama: Status sistem
  if (!sensorsEnabled) {
    lcd.print("SENSOR MATI");
  } else if (gateOpen) {
    lcd.print("PALANG TERBUKA");
  } else {
    lcd.print("PAlANG TERTUTUP");
  }
  
  // Baris kedua: Informasi detail
  lcd.setCursor(0, 1);
  if (!sensorsEnabled) {
    lcd.print("Mode Manual");
  } else if (gateOpen) {
    lcd.print("Kendaraan ke : ");
    lcd.print(vehicleCount);
  } else {
    if (distance < 30 && distance > 0) {
      lcd.print("Jarak: ");
      lcd.print(distance);
      lcd.print(" cm");
    } else {
      lcd.print(" ");
    }
  }
}

void toggleSensors() {
  sensorsEnabled = !sensorsEnabled;
  
  // Update LED indikator sensor
  digitalWrite(sensorLED, sensorsEnabled ? HIGH : LOW);
  
  // Feedback audio
  if (sensorsEnabled) {
    // Sensor dinyalakan - nada naik
    tone(buzzer, 1000, 200);
    delay(250);
    tone(buzzer, 1500, 200);
    delay(250);
    tone(buzzer, 2000, 200);
    
    lcd.clear();
    lcd.print("SENSOR AKTIF");
    lcd.setCursor(0, 1);
    lcd.print("Mode Otomatis");
  } else {
    // Sensor dimatikan - nada turun
    tone(buzzer, 2000, 200);
    delay(250);
    tone(buzzer, 1500, 200);
    delay(250);
    tone(buzzer, 1000, 200);
    
    lcd.clear();
    lcd.print("SENSOR MATI");
    lcd.setCursor(0, 1);
    lcd.print("Mode Manual");
  }
  
  delay(1500); // Tampilkan pesan sebentar
  
  Serial.print("Sensors ");
  Serial.println(sensorsEnabled ? "ENABLED" : "DISABLED");
}

void resetSystem() {
  vehicleCount = 0;
  gateOpen = false;
  manualMode = false;
  servoMotor.write(0);
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledGreen, LOW);
  
  lcd.clear();
  lcd.print("SISTEM RESET");
  lcd.setCursor(0, 1);
  lcd.print("Siap Digunakan");
  delay(2000);
  
  updateLCD();
}
