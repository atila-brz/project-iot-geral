#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 4    

#define DHTTYPE DHT22   

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  Serial.println("DHT22 Sensor Teste!");

  dht.begin();
}

void loop() {
  delay(2000); 

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Falha na leitura do sensor DHT22!");
    return;
  }

  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Umidade: ");
  Serial.print(h);
  Serial.print(" %\t");
  
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.print(" *C ");
  
  Serial.print("Sensacao Termica: ");
  Serial.print(hic);
  Serial.println(" *C");
}