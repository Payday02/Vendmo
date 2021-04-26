/**
 * Project: Vendmo
 * File name: Vendmo.ino
 * Description:  Blynk event handler and MDB interface
 *   
 * @author Paden Davis
 * @email pdavis77@gatech.edu
 *   
 */

#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <Regexp.h>
#include "mdb.h"
#include "usart.h"
#include "uplink.h"

#define RESET_PIN 13 // bridge this to 'RESET' pin
#define SS_PIN_W5100  10 // W5100 (eth shield) SS Pin

const char auth[] = "";   //Enter Blynk Token Here
WidgetTerminal terminal(V0);
WidgetLED led1(V10);
MatchState ms; // regex
EthernetClient client;
extern uint8_t mdb_state;
uint8_t state;
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

  state = mdb_state;
  terminal.print("starting mdb_state: ");
  terminal.println(mdb_state);
  
}

void loop() {
  Blynk.run();
  mdb_cmd_handler();
  uplink_cmd_handler();

  
//  if (state != mdb_state) {
//   terminal.print("mdb_state: ");
//   terminal.println(mdb_state);
//  }
//  state = mdb_state;
//  terminal.print("mdb_state: ");
//  terminal.println(mdb_state);
}

char* dtoa(double dN, char *cMJA, int iP) {
  char *ret = cMJA;
  long lP=1;
  byte bW=iP;
  
  while (bW>0) { 
    lP=lP*10;
    bW--;  
  }
  
  long lL = long(dN);
  double dD=(dN-double(lL))* double(lP); 
  if (dN>=0) {
    dD=(dD + 0.5);  
  } 
  else {
    dD=(dD-0.5); 
  }
  
  long lR=abs(long(dD));
  lL=abs(lL);  
  if (lR==lP) {
    lL=lL+1;
    lR=0;  
  }
  if ((dN<0) & ((lR+lL)>0)) {
    *cMJA++ = '-';  
  } 
  ltoa(lL, cMJA, 10);
  if (iP>0) {
    while (*cMJA != '\0') {
      cMJA++; 
    }
    *cMJA++ = '.';
    lP=10; 
    while (iP>1) {
      if (lR< lP) {
        *cMJA='0';
        cMJA++;
      }
      lP=lP*10;
      iP--; 
    }
    ltoa(lR, cMJA, 10);
  }  
  return ret; 
}
/*
 * TODO docstring
 * maybe uint16_t instead of double?
 */
void vend(double amount) {
  char* amnt;
  dtoa(amount, amnt, 2);
  cmd_approve_vend(amnt);
}

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
    vend(manualAmount);
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
    vend(max(payment, 10.00)); // cap at ten bucks
  } else {
    terminal.println("WARNING: HTTP REQUEST INVALID");
  }
  terminal.flush();
}

void reset() {
  digitalWrite(RESET_PIN, LOW);
}
