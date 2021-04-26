#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <Regexp.h>
#include "mdb.h"
#include "usart.h"
#include "uplink.h"

#define RESET_PIN 13 // bridge this to 'RESET' pin
#define SS_PIN_W5100  10 // W5100 (eth shield) SS Pin

const double denominations[] = {0.25, 0.10, 0.05}; // Q, D, N

const char auth[] = "3a27b4203e1245d99ee024ec3ab7022c";
WidgetTerminal terminal(V0);
WidgetLED led1(V10);
MatchState ms; // regex
EthernetClient client;

volatile double manualAmount = 0;

void setup() {
  digitalWrite(RESET_PIN, HIGH); // since it's active-low
  pinMode(RESET_PIN, OUTPUT);

  setup_usart(0,38400,8,'N',1);
  setup_usart(1,9600,9,'N',1);
  
  Blynk.begin(auth);
  terminal.clear();
  terminal.println("Blynk connection established.");
  terminal.flush();

  sei(); // idek what this really changes. You look it up.
}

void loop() {
  mdb_cmd_handler();
  uplink_cmd_handler();

  Blynk.run();
}

/*
 * Handler for two Cashless Device active states:
 * ENABLED -- device is waiting for a new card
 * VEND    -- device is busy making transactions with the server
 */
//void sessionHandler(void)
//{
//    switch(CSH_GetDeviceState())
//    {
//        case CSH_S_ENABLED:
//          RFID_readerHandler();
//          break;
//        case CSH_S_VEND:
//          transactionHandler();
//          break;
//    }
//    char c = Debug.read();
//    if (c == 0x30)
//        CSH_SetPollState(CSH_END_SESSION);
//}


/*
 * TODO docstring
 * maybe uint16_t instead of double?
 */
//void vend(double amount) {
//  terminal.println("pretending to vend");
//}

BLYNK_WRITE(V0) { // blynk terminal inbound data
  String cmd = param.asStr();
  if (cmd.equalsIgnoreCase("reset")) {
    reset();
  } else if (cmd.equalsIgnoreCase("ping")) {
    terminal.println("pong from arduino");
    // TODO: pong from python daemon
  }
  terminal.flush();
}

BLYNK_WRITE(V1) { // number box
  manualAmount = param.asDouble();
  terminal.print("Manual vend amount changed to $");
  terminal.println(manualAmount);
  terminal.flush();
}

BLYNK_WRITE(V2) { // send button
  if (param.asInt()) { // when pressed
    terminal.println("Vending manually:");
    terminal.flush();
//    vend(manualAmount);
  }
}

BLYNK_WRITE(V9) { // Handling HTTP request
  char * httpRequest = (char *) param.asStr();
  terminal.println("HTTP request received:");
  terminal.println(httpRequest);
  char buf [128];
  ms.Target(httpRequest);
  char result = ms.Match ("(.*) > (.*) paid you $([%d%.]*)", 0);
  if (result == REGEXP_MATCHED) {
    String timestamp = ms.GetCapture(buf, 0);
    String sender = ms.GetCapture(buf, 1);
    String strPayment = ms.GetCapture(buf, 2);
    double payment = strPayment.toDouble();
    
    Blynk.virtualWrite(V5, sender, payment, timestamp); // autofill form
//    vend(max(payment, 10.00)); // cap at ten bucks
  } else {
    terminal.println("WARNING: HTTP REQUEST INVALID");
  }
  terminal.flush();
}

void reset() {
  digitalWrite(RESET_PIN, LOW);
}
