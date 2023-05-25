#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

// TODO -- Define WiFi credentials
const char* ssid = "Horias phone";
const char* password = "horia2307";

// TODO -- BLE server name
#define bleNumeServer "SoulCynics"
   String new_ID;
String base_URL = "http://proiectia.bogdanflorea.ro/api/rick-and-morty/";
bool deviceConnected = false;

  // This function deals with receiving the json from the mobile application, its unrealization in a string,
  // processing it and forwarding, to the following methods, the type of toDo required by the application.
String phoneResponse(BLECharacteristic *characteristic) {
 //We read the string received from the application and store the text in the variable above
   String dataFromApp = characteristic->getValue().c_str();

   // Create a relatively small dynamic json document to deserialize the text received from the mobile application
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, dataFromApp);

  // Testam conexiunea
  if (error) {
  
    return "none";
  }
  Serial.println();
 // In the string type variable action we store the actual toDo received from the application 
  // The action can be of two types: fetchData and fetchDetails
  
  String action = doc["action"];
  if (action == "fetchData") {
     // If we receive the "fetchData" statement it returns the name of the action further
    Serial.println(action);
    return action;
  }
  else if (action == "fetchDetails") {
     // If we get the "fetchDetails" statement it returns the name of the action
    String id = doc["id"];
    Serial.print(action);
    Serial.print(" : ");
    Serial.println(id);
    new_ID = id;
    return action;
  }

}
String httpGETRequest(String aux) {
// We declare an HTTPClient object to use the connection to the API and to make the corresponding request
  HTTPClient http;

  // The path variable contains the base link to which the rest of the link is concatenated
   // depending on the current action, which can be "characters" or "character /" + id_charcter
  String path = base_URL + aux;
  String payload = "";
  Serial.print("Attempting to connect to: ");
  Serial.println(path);

  //Specify the URL
  http.setTimeout(12000);
  // Connect to API
  http.begin(path);

  //Make the request
  int httpCode = http.GET();
// We save the serialized string in the payload variable that we will return later
  if (httpCode == 200) { //Check for the returning code

    payload = http.getString();
    Serial.println("Successful request");
  }

  else {

    Serial.print("Error on HTTP request: ");
    Serial.println(httpCode);
  }

  http.end(); //Free the resources

  return payload;
}

// This function deals with the deserialization of the data received from the link that contains the information about a certain character.
// Some of these will be saved in a new StaticJsonDocument object.
// After serializing a string with the newly created object, it will be forwarded to the mobile application, via bluetooth.

void Data_To_App(String payload,BLECharacteristic *characteristic) {
  StaticJsonDocument <768> JSONDocument;
  DeserializationError error = deserializeJson(JSONDocument, payload.c_str());

  if (error) {
    Serial.print("Problem with fetchDetails: ");
    Serial.println(error.c_str());

  } else {
   String responseString; 
      String id_cpy=JSONDocument["id"].as<String>();
      String name_cpy=JSONDocument["name"].as<String>();
      String image_cpy=JSONDocument["image"].as<String>();
      String status_cpy=JSONDocument["status"].as<String>();
      String species_cpy=JSONDocument["species"].as<String>();
      String origin_cpy=JSONDocument["origin"]["name"].as<String>();
      String location_cpy=JSONDocument["location"]["name"].as<String>();

      String description_cpy="Status: "+status_cpy+"\n"+"Species: "+species_cpy+"\n"+"Origin: "+origin_cpy+"\n"+"Location: "+location_cpy;
    StaticJsonDocument <512> newJSONDocument;//https://arduinojson.org/v6/assistant

    JsonObject object = newJSONDocument.to<JsonObject>();

    object["id"] = id_cpy;
    object["name"] = name_cpy;
    object["image"] = image_cpy;
    object["description"] = description_cpy;


    serializeJson(newJSONDocument, responseString);
      characteristic->setValue(responseString.c_str());
      characteristic->notify();
   
     Serial.println(" ++++++++++++++++++++++++++++++++++++++++++++++++++ ");

      Serial.print(id_cpy);
      Serial.print(" (");
      Serial.print(name_cpy);
      Serial.print("): ");
      Serial.println(image_cpy);
      Serial.println("Description:");
      Serial.println(description_cpy);

      Serial.println(" ++++++++++++++++++++++++++++++++++++++++++++++++++ ");
   

  }
}



void sendDataToApp(String payload,BLECharacteristic *characteristic) {
  DynamicJsonDocument JSONDocument(8192);
  //Deserealize the received string in a json document
  DeserializationError error = deserializeJson(JSONDocument, payload.c_str());

  if (error) {
    Serial.print("Problem with fetchData: ");
    Serial.println(error.c_str());

  } else {
    // Define a JsonArray from the JSONDocument, since the JSONString is an array of objects
    JsonArray list = JSONDocument.as<JsonArray>();

    // Iterate the JsonArray array (see docs for the library)
    int index = 1;
    for (JsonVariant value : list) {
      JsonObject listItem = value.as<JsonObject>();
      // Get the current item in the iterated list as a JsonObject

      String id_cpy = listItem["id"].as<String>();
      String name_cpy = listItem["name"].as<String>();
      String image_cpy = listItem["image"].as<String>();
      String textCh = listItem["text"].as<String>();
      String responseString;
  
      StaticJsonDocument <256> newJSONDocument;//https://arduinojson.org/v6/assistant

      JsonObject object = newJSONDocument.to<JsonObject>();

      object["id"] = id_cpy;
      object["name"] = name_cpy;
      object["image"] = image_cpy;
      serializeJson(newJSONDocument, responseString);
// Send the string to the application via bluetooth
      characteristic->setValue(responseString.c_str());
      characteristic->notify();
      Serial.print(index);
      Serial.print(" ");
      Serial.println(responseString);

      delay(100);
      index++;
    }
  }
}
// TODO --  Generate a unique UUID for your Bluetooth service
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

// BEGIN CHARACTERISTICS
// FOR THE FOLLOWING LINES CHANGE ONLY THE UUID of the characteristic
// Define two caracteristics with the properties: Read, Write (with response), Notify
// Use th above link for generating UUIDs
BLECharacteristic indexCharacteristic(
  "ca73b3ba-39f6-4ab3-91ae-186dc9577d99", // <-- TODO -- Change Me
  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
);
// Define a descriptor characteristic
// IMPORTANT -- The list characteristc must have the descriptor UUID 0x2901
BLEDescriptor *indexDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));

BLECharacteristic detailsCharacteristic(
  "183f3323-b11f-4065-ab6e-6a13fd7b104d", // <-- TODO -- Change Me
  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
);
// Define a descriptor characteristic
// IMPORTANT -- The details characteristc must have the descriptor UUID 0x2902
BLEDescriptor *detailsDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
// END CHARACTERISTICS

// Setup callbacks onConnect and onDisconnect (no change necessary)
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};
// End setup callbacks onConnect and onDisconnect (no change necessary)

// Setup callbacks for charactristics
// IMPORTANT -- both caracteristics can use the same callbacks class 
// or you can define a different class for each characteristic containing the onWrite method
// The onWrite callback method is called whend data is received by the ESP32
// This is where you will wrwite you logic, according to the API specs
class CharacteristicsCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
      // Get characteristic value sent from the app, according to the specs
     
     
        String Phone_response = phoneResponse(characteristic);
   
        String payload = "";
        if (Phone_response == "fetchData") {
          payload = httpGETRequest("characters");
          sendDataToApp(payload,characteristic);
          Serial.println();
          Serial.println("Just waiting for new action");

        }
         if (Phone_response == "fetchDetails") {
          payload = httpGETRequest("character/" +new_ID);
          Data_To_App(payload,characteristic);        
          Serial.println();
          Serial.println("Just waiting for new action");  

        }
       
      }
};

void setup() {
  // Start serial communication 
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Print local IP address and start web server
  Serial.println("WiFi connected");

  BLEDevice::init(bleNumeServer);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  // Set server callbacks
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *bmeService = pServer->createService(SERVICE_UUID);

  // Create BLE characteristics and descriptors
  // List
  bmeService->addCharacteristic(&indexCharacteristic);  
  indexDescriptor->setValue("Get data list");
  indexCharacteristic.addDescriptor(indexDescriptor);
  indexCharacteristic.setValue("Get data List");
  // Set chacrateristic callbacks
  indexCharacteristic.setCallbacks(new CharacteristicsCallbacks());

  // Details
  bmeService->addCharacteristic(&detailsCharacteristic);  
  detailsDescriptor->setValue("Get data details");
  detailsCharacteristic.addDescriptor(detailsDescriptor);
  detailsCharacteristic.setValue("Get data details");
  // Set chacrateristic callbacks
  detailsCharacteristic.setCallbacks(new CharacteristicsCallbacks());
  
  // Start the service
  bmeService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  // END DON'T CHANGE
}

void loop() {
  // Nothing to do here since the code relies on callbacks and notifications
}
