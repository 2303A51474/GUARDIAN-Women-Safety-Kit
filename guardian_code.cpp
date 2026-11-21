#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// ================= GPS SETUP =================
SoftwareSerial sgps(5, 6);  // Arduino RX, TX
TinyGPSPlus gps;

float latitude = 0.0;
float longitude = 0.0;
bool gpsFixAvailable = false;

// ================= GSM SETUP =================
SoftwareSerial sgsm(2, 3);

// Emergency phone number
const char phoneNumber[] = "+917396102528";

// ================= BUTTONS =================
const int emergencyButton = 9;  // Manual emergency button
const int confirmButton = 7;     // New 5-second confirmation button

// ================= BUZZER & LED =================
const int buzzerPin = 10;
const int ledPin = 13;

// ================= TIMER =================
const unsigned long CONFIRM_TIME = 5000UL;
unsigned long startupTime;

bool confirmationReceived = false;
bool missingAlertSent = false;
bool emergencyButtonLocked = false;

// ================= SETUP =================
void setup() {
  Serial.begin(9600);

  sgps.begin(9600);
  sgsm.begin(9600);

  pinMode(emergencyButton, INPUT_PULLUP);
  pinMode(confirmButton, INPUT_PULLUP);

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  startupTime = millis();

  Serial.println("================================");
  Serial.println(" GPS GSM EMERGENCY ALERT SYSTEM");
  Serial.println("================================");
  Serial.println("Press confirmation button");
  Serial.println("within 5 seconds after startup.");
  Serial.println("Otherwise, a missing-kit alert");
  Serial.println("will be sent.");
}

// ================= GPS UPDATE =================
void updateGPS() {
  sgps.listen();

  while (sgps.available()) {
    gps.encode(sgps.read());
  }

  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    gpsFixAvailable = true;
  }
}

// ================= SEND SMS =================
void sendSMS(const char* message) {
  sgsm.listen();

  sgsm.println("AT");
  delay(500);

  sgsm.println("AT+CMGF=1");
  delay(500);

  sgsm.print("AT+CMGS=\"");
  sgsm.print(phoneNumber);
  sgsm.println("\"");
  delay(1000);

  sgsm.println(message);
  sgsm.println();

  sgsm.print("Location: ");

  if (gpsFixAvailable) {
    sgsm.print("https://maps.google.com/?q=");
    sgsm.print(latitude, 6);
    sgsm.print(",");
    sgsm.println(longitude, 6);
  } else {
    sgsm.println("GPS fix unavailable.");
  }

  delay(500);
  sgsm.write(26);  // Ctrl+Z to send SMS
  delay(5000);

  Serial.println("SMS sending command completed.");
}

// ================= MAKE CALL =================
void makeCall() {
  sgsm.listen();

  sgsm.print("ATD");
  sgsm.print(phoneNumber);
  sgsm.println(";");

  delay(15000);

  sgsm.println("ATH");
  delay(1000);

  Serial.println("Call ended.");
}

// ================= ALERT FUNCTION =================
void triggerAlert(const char* message) {
  digitalWrite(buzzerPin, HIGH);
  digitalWrite(ledPin, HIGH);

  Serial.println("EMERGENCY ALERT!");

  sendSMS(message);
  makeCall();

  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  Serial.println("Alert sequence completed.");
}

// ================= MAIN LOOP =================
void loop() {
  // Read GPS data
  updateGPS();

  // --------------------------------
  // 5-SECOND STARTUP CONFIRMATION
  // --------------------------------
  if (!confirmationReceived && !missingAlertSent) {

    // Button pressed within first 5 seconds
    if (digitalRead(confirmButton) == LOW &&
        millis() - startupTime < CONFIRM_TIME) {

      confirmationReceived = true;

      Serial.println("Confirmation received!");
      Serial.println("Missing-kit alert cancelled.");
    }

    // No confirmation within 5 seconds
    if (!confirmationReceived &&
        millis() - startupTime >= CONFIRM_TIME) {

      missingAlertSent = true;

      Serial.println("Confirmation timeout!");
      Serial.println("Sending missing-kit alert...");

      triggerAlert(
        "MISSING KIT ALERT! No confirmation "
        "button press within 5 seconds of startup."
      );
    }
  }

  // --------------------------------
  // MANUAL EMERGENCY BUTTON
  // --------------------------------
  bool pressed = digitalRead(emergencyButton) == LOW;

  if (pressed && !emergencyButtonLocked) {
    emergencyButtonLocked = true;

    triggerAlert(
      "EMERGENCY ALERT! Help needed immediately."
    );
  }

  // Allow another press after release
  if (!pressed) {
    emergencyButtonLocked = false;
  }

  delay(30);
}