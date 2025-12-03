#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>
#include "Memory.h"
#include "Message.h"
#include "Contact.h"
#include "Entropy.h"
#include "EEPROM.h"
#include "LCD.h"
#include <SPI.h>
#include "buzzer.h"

LCD myscreen;  // Create LCD object
Buzzer mybuzzer(4);  // Buzzer on pin 4

// Generate UUID at runtime using Entropy
uint8_t nodeUUID[Contact::UUID_LEN];

//Hint: try coding first with hard wired name and uuid and then make it random (in case randomizing gives you any error).
char nodeName[10] = {'Y','O','U',' ',' ',' ',' ',' ',' ',' '};

Contact me;              // default-constructed, we’ll set name + UUID in setup
Memory  memory;          // default-constructed, schema handled in setup

// RF24 radio on CE=A1, CSN=A2
RF24 myradio(A1,A2);

// For early testing you can use a fixed peer address (another board)
uint8_t peerUUID[Message::UUID_LENGTH] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01};


// ----------------- Helper: show name + UUID -----------------

void showIdentity() {
    myscreen.clear();
    myscreen.setCursor(0, 0);
    myscreen.print("Name:");
    for (uint8_t i = 0; i < Contact::NAME_LEN; ++i) {
        myscreen.print(nodeName[i]);
    }

    myscreen.setCursor(0, 1);
    myscreen.print("UUID:");

    Serial.print("Node name: ");
    for (uint8_t i = 0; i < Contact::NAME_LEN; ++i) {
        Serial.print(nodeName[i]);
    }
    Serial.println();

    Serial.print("Node UUID: 0x");
    for (uint8_t i = 0; i < Contact::UUID_LEN; ++i) {
        if (nodeUUID[i] < 0x10) Serial.print('0');
        Serial.print(nodeUUID[i], HEX);

        // Also show first 3 bytes on LCD to fit 16 chars
        if (i < 3) {
            if (nodeUUID[i] < 0x10) myscreen.print('0');
            myscreen.print(nodeUUID[i], HEX);
        }
    }
    Serial.println();
}

// -------------------- Helper: radio init --------------------

void initRadio() {
    if (!myradio.begin()) {
        Serial.println("Radio hardware not responding!");
        myscreen.clear();
        myscreen.setCursor(0, 0);
        myscreen.print("Radio ERROR");
        // Error buzzer loop
        while (true) {
            mybuzzer.bback();
            delay(400);
        }
    }

    myradio.setPALevel(RF24_PA_LOW);
    myradio.setDataRate(RF24_250KBPS);
    myradio.setChannel(100);

    // Use our node UUID as the reading address (5 bytes) 
    myradio.openReadingPipe(1, nodeUUID);
    myradio.startListening();

    Serial.println("Radio initialized, listening...");
    myscreen.clear();
    myscreen.setCursor(0, 0);
    myscreen.print("Radio OK");
    delay(800);
}

// -------------------- setup() --------------------

void setup() {
    Serial.begin(9600);
    printf_begin();  // optional RF24 debug prints

    // LCD init & welcome
    myscreen.begin(16, 2);
    myscreen.clear();
    myscreen.setCursor(0, 0);
    myscreen.print("Welcome!");
    myscreen.setCursor(0, 1);
    myscreen.print("Morse Beeper");
    mybuzzer.bselect();
    delay(1500);
    myscreen.clear();

    // --- Generate random UUID using Entropy --- 
    Entropy.initialize();
    for (uint8_t i = 0; i < Contact::UUID_LEN; ++i) {
        nodeUUID[i] = Entropy.randomByte();
    }

    // Configure our Contact object with the random UUID + fixed name
    me.setUUID(nodeUUID);
    me.setName(nodeName);

    // --- Initialize EEPROM schema (flags, node contact, counters) 
    if (!memory.hasSchema()) {
        memory.setSchema();
        memory.clearContacts();
        memory.clearMessages();
        memory.saveNodeInformation(me);
    }
    // (Later you can also read back with memory.getNodeUUID()/getNodeName().)

    // Show name + UUID
    showIdentity();
    delay(2000);

    // Radio init
    initRadio();

    // Ready prompt
    myscreen.clear();
    myscreen.setCursor(0, 0);
    myscreen.print("Press buttons");
    myscreen.setCursor(0, 1);
    myscreen.print("SEL=demo");
}

// -------------------- loop() --------------------

void loop() {
    // ------------ 1) Handle radio receive (stub for now) ------------

    if (myradio.available()) {
        Message incoming;
        
        // Read latest message object from the radio
        while (myradio.available()) {
            myradio.read(&incoming, sizeof(Message));
        }

        // Decode Morse payload to a string 
        char* text = incoming.getPayloadString();

        // Display on LCD
        myscreen.clear();
        myscreen.setCursor(0, 0);
        myscreen.print("Msg received!");
        mybuzzer.bselect();

        Serial.println("Radio data received.");

        delay(1500);

        // Free allocated string to avoid memory leak
        delete[] text;

        // Return idle screen
        myscreen.clear();
        myscreen.setCursor(0, 0);
        myscreen.print("Press buttons");
        myscreen.setCursor(0, 1);
        myscreen.print("SEL=demo");
    }

    // ------------ 2) Handle buttons via LCD.getButtonPress() ------------ 

    Button b = myscreen.getButtonPress();

    if (b != NONE) {
        myscreen.setCursor(0, 1);
        myscreen.print("                ");  // clear bottom row
        myscreen.setCursor(0, 1);
    }

    switch (b) {
        case LEFT:
            myscreen.print("LEFT");
            mybuzzer.bscroll();
            break;

        case RIGHT:
            myscreen.print("RIGHT");
            mybuzzer.bscroll();
            break;

        case UP: {
            // Show counts using Memory (once implemented) 
            unsigned short numContacts  = memory.getNumberContacts();
            unsigned short numMessages  = memory.getNumberMessages();
            char buf[17];
            snprintf(buf, sizeof(buf), "C:%u M:%u", numContacts, numMessages);
            myscreen.print(buf);
            mybuzzer.bscroll();
            break;
        }

        case DOWN:
            myscreen.print("DOWN");
            mybuzzer.bback();
            break;

        case SELECT: {
    myscreen.print("Sending...");
    mybuzzer.bselect();

    const char* morse = ".-.-";   // test message

    Message msg(nodeUUID, peerUUID, morse);

    myradio.stopListening();
    myradio.openWritingPipe(peerUUID);
    bool ok = myradio.write(&msg, sizeof(Message));
    myradio.startListening();

    myscreen.clear();
    myscreen.setCursor(0,0);
    myscreen.print(ok ? "Sent OK" : "Send FAIL");
    delay(1500);

    myscreen.clear();
    myscreen.setCursor(0, 0);
    myscreen.print("Press buttons");
    myscreen.setCursor(0, 1);
    myscreen.print("SEL=demo");
    break;
}

        case NONE:
        default:
            // no button press
            break;
    }

    delay(30);  // small delay to avoid spamming
}