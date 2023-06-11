#include <EthernetUdp2.h>
#include <EthernetClient.h>
#include <Dhcp.h>
#include <EthernetServer.h>
#include <Ethernet2.h>
#include <util.h>
#include <Dns.h>
#include <Twitter.h>

#define KNOB_1 32
#define KNOB_2 34
#define KNOB_3 36
#define KNOB_4 38
#define KNOB_LOCK 40

int KNOB_STATE;

bool useDHCP = false;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x2C, 0xF7, 0xF1, 0x08, 0x18, 0x06
};

IPAddress ip(10, 1, 20, 110);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(KNOB_1, INPUT_PULLUP);
  pinMode(KNOB_2, INPUT_PULLUP);
  pinMode(KNOB_3, INPUT_PULLUP);
  pinMode(KNOB_4, INPUT_PULLUP);
  pinMode(KNOB_LOCK, OUTPUT);

  // start the Ethernet connection and the server:
  Serial.println("Not using DHCP");
  Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());
  
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  // listen for incoming Ethernet connections:
  listenForEthernetClients();

  // Maintain DHCP lease
  if (useDHCP) {
    Ethernet.maintain();
  }
  
  // put your main code here, to run repeatedly:
  KNOB_STATE = digitalRead(KNOB_1) + digitalRead(KNOB_2) + digitalRead(KNOB_3) + digitalRead(KNOB_4);
  Serial.print("KNOB: ");
  Serial.println(KNOB_STATE);
  if (KNOB_STATE == 0) {
    digitalWrite(KNOB_LOCK, HIGH);
  }
}

void reset() {
  digitalWrite(KNOB_LOCK, LOW);
}

String statusString(int state) {
  String returnString = "Knobs Left: " + state;
  return returnString;
}

void processRequest(EthernetClient& client, String requestStr) {
  Serial.println(requestStr);

  // Send back different response based on request string
  if (requestStr.startsWith("GET /status")) {
    Serial.println("polled for status!");
    writeClientResponse(client, statusString(KNOB_STATE));
  } else if (requestStr.startsWith("GET /reset")) {
    Serial.println("Room reset");
    reset();
    writeClientResponse(client, "ok");
  } else if (requestStr.startsWith("GET /unlock")) {
    Serial.println("Unlocked");
    writeClientResponse(client, "Unlocked");
  } else {
    writeClientResponseNotFound(client);
  }
}

/*
 * HTTP helper functions
 */

void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    // Grab the first HTTP header (GET /status HTTP/1.1)
    String requestStr;
    boolean firstLine = true;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          processRequest(client, requestStr);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          firstLine = false;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;

          if (firstLine) {
            requestStr.concat(c);
          }
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}


void writeClientResponse(EthernetClient& client, String bodyStr) {
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Access-Control-Allow-Origin: *");  // ERM will not be able to connect without this header!
  client.println();
  client.print(bodyStr);
}


void writeClientResponseNotFound(EthernetClient& client) {
  // send a standard http response header
  client.println("HTTP/1.1 404 Not Found");
  client.println("Access-Control-Allow-Origin: *");  // ERM will not be able to connect without this header!
  client.println();
}

