#include "Memory.h"
#include "EEPROM.h"
#include <Arduino.h>
#include <string.h>

EEPROM myEEPROM;

//EEPROM ADDRESS CONSTANTS
//Flags
#define ADDR_INT_FLAG       0x000   //3 Bytes: 0xCOFFEE
#define ADDR_CONTACT_FLAG   0X018   //2 Bytes: 0xFACE
#define ADDR_MESSAGE_FLAG   0x171   //2 Bytes: 0xCA11

//Counters
#define ADDR_CONTACT_COUNT  0x020   //1 Byte
#define ADDR_MESSAGE_COUNT  0x173   //1 Byte

//Stroage Addresses
#define ADDR_NODE_CONTACT   0x003   //15 Bytes
#define ADDR_CONTACT_LIST   0x021   //10 * 15 Bytes
#define ADDR_MESSAGE_LIST   0x174   //20 * 13 Bytes
#define ADDR_MESSAGE_OFFSET 0x434   //1 Byte

//Constructors
//Default Constructor
Memory::Memory(){

}
//Constructor to initialize with node's contact info
Memory::Memory(Contact node) {
  if(!hasSchema()) {
    saveNodeInformation(node);
    setSchema();
  }
}

//Checks for the three flag values (0xCOFFEE, 0xFACE, 0xCA11)
bool Memory::hasSchema(){
  //Check ADDR_INIT_FLAG is 3 Bytes (0xCOFFEE)
  if (EEPROM::read(ADDR_INIT_FLAG) != 0xC0 || EEPROM::read(ADDR_INIT_FLAG + 1) != 0xFF || EEPROM::read(ADDR_INIT_FLAG + 2) != 0xEE) {
    return false;
  } 
  //Check ADDR_CONTACT_FLAG is 2 Bytes (0xFACE)
  if (EEPROM::read(ADDR_CONTACT_FLAG) != 0xFA || EEPROM::read(ADDR_CONTACT_FLAG) != 0xCE) {
    return false;
  }
  //Check ADDR_MESSAGE_FLAG is 2 Bytes (0xCA11)
  if (EEPROM::read(ADDR_MESSAGE_FLAG) != 0xCA || EEPROM::read(ADDR_MESSAGE_FLAG + 1) != 0x11) {
    return false;
  }
  //All Flags are correct
  return true;    
}

//Writes the Flag Values
void Memory::setSchema(){
    //Write to the EEPROM Flag for 0xCOFFEE
    EEPROM::write(ADDR_INIT_FLAG, 0xC0);
    EEPROM::write(ADDR_INIT_FLAG, 0xFF);
    EEPROM::write(ADDR_INIT_FLAG, 0xEE);

    //Write to the EEPROM Flag for 0xFACE
    EEPROM::write(ADDR_CONTACT_FLAG, 0xFA);
    EEPROM::write(ADDR_CONTACT_FLAG, 0xCE);

    //Write to the EEPROM flag for 0xCA11
    EEPROM::write(ADDR_MESSAGE_FLAG, 0xCA);
    EEPROM::write(ADDR_MESSAGE_FLAG, 0x11);

    //Start the Counters @ 0
    EEPROM::write(ADDR_CONTACT_COUNT, 0);
    EEPROM::write(ADDR_MESSAGE_COUNT, 0);

}

//Clear all the contacts 
void Memory::clearContacts(){
  //Clears counter
  EEPROM::write(ADDR_CONTACT_COUNT, 0);

}

void Memory::clearMessages(){
  //Clears message and offset counters
  EEPROM::write(ADDR_MESSAGE_COUNT, 0);
  EEPROM::write(ADDR_MESSAGE_OFFSET, 0);
}

unsigned short Memory::getMessagePointerOffset(){
  //Gets message offset pointer
  return EEPROM::read(ADDR_MESSAGE_OFFSET);

} 


//Save device's own contact info (Name with UUID)
void Memory::saveNodeInformation(Contact contact){
  //Create pointers for UUID and Name
  const uint8_t* uuid_ptr = contact.getUUID();
  char* name_ptr = contact.getName();

  //5 Bytes for UUID
  for(int i = 0; i < Contact.UUID_LEN; i++) {
    EEPROM::write(ADDR_NODE_CONTACT + i, uuid_ptr[i]);
  }
  //10 Bytes for Name, account for null terminator
  for(int i = 0; i < Contact.NAME_LEN; i++) {
    EEPROM::write(ADDR_NODE_CONTACT + Contact::UUID_LEN + i, name_ptr[i]);
  }

}

//Read 5 Byte UUID from EEPROM
unsigned char* Memory::getNodeUUID(){
  //Create static buffer
  static unsigned char uuid_bfr[Contact::UUID_LEN];
  //Increment and store in static buffer
  for(int i = 0; i < Contact::UUID_LEN; i++) {
    uuid_bfr[i] = EEPROM::read(ADDR_NODE_CONTACT + i);
  }

  return uuid_buffer;
}

//Read 10 Byte Name from EEPROM
char* Memory::getNodeName(){
  //Create static buffer
  static char name_bfr[Contact::NAME_LEN];
  //Increment and store in static buffer
  for(int i = 0; i < Contact::NAME_LEN; i++) {
    name_bfr[i] = EEPROM::read(ADDR_NODE_CONTACT + Contact::UUID_LEN + i);
  }

  return name_bfr;
}

//Get the number count
unsigned short Memory::getNumberContacts(){
  //Returns contact count
  return EEPROM::read(ADDR_CONTACT_COUNT);
}

//Get the message count
unsigned short Memory::getNumberMessages(){
  //Returns message count
  return EEPROM::read(ADDR_MESSAGE_COUNT);

}

//Get memory offset
Contact Memory::getContact(unsigned short index){
  //Returns memory offset
  return EEPROM::read(ADDR_MESSAGE_OFFSET);
    
}

//Get message object
Message Memory::getMessage(unsigned short index){
    //Check index boundary
    if (index >= getNumberMessages()) {
      //Return empty message
      return Message();
    }

    //Calculate start address (Message size is 13 Bytes)
    uint16_t addr = ADDR_MESSAGE_LIST + (index * 13);

    //Read 13 Byte Message data
    uint8_t = [13];
    for(int i = 0; i < 13; i++) {
      buffer[i] = EEPROM::read(addr + i);
    }

  //Assign recieved data
  uint8_t from_uuid[5];
  uint8_t to_uuid[5];
  uint16_t payload;
  uint8_t length;
  //Get data from buffer
  memcpy(from_uuid, buffer, 5);
  memcpy(to_uuid, buffer + 5, 5);
  payload = buffer[10] | (buffer[11] << 8);
  length = buffer[12];

  //Create and return message object
  return Message(from_uuid, to_uuid, payload, length);
}

//Saves contact object in EEPROM contact list
bool Memory::saveContact(Contact contact){
  unsigned short count = getNumberContacts();

  //Check if list is full (max 10)
  if(count >= MAX_CONTACTS) {
    //List is full
    return false;
  }
  //Calculate new start address
  uint16_t addr = ADDR_CONTACT_LIST + (count * 15); 

  //Pointers for name and UUID addresses
  const uint8_t* uuid_ptr = contact.getUUID();
  char* name_ptr = contact.getName();

  //Write UUID 5 Bytes
  for(int i = 0; i < Contact::UUID_LEN; i++) {
    EEPROM::write(addr + i, uuid_ptr[i]);
  }

  //Write Name 10 Bytes
  for(int i = 0; i < Contact::NAME_LEN; i++) {
    EEPROM::write(addr + Contact::UUID_LEN + i, name_ptr[i]);
  } 

  //Increment contact counter
  EEPROM::write(ADDR_CONTACT_COUNT, count + 1);

  return true;
}

//Saves message object to EEPROM message list
bool Memory::saveMessage(Message message){

  unsigned short count = getNumberMessage();
  unsigned short count = getMessagePointerOffset();

  //Check message size (20 max)
  if(count < MAX_MESSAGES) {
    //Save at the end of list with current offset
    offset = count * 13;      //Offset is index * size
    EEPROM::write(ADDR_MESSAGE_LIST, count + 1);
  }
  else
  {
    //Setup offset
    offset = offset * 13
  }
  //Calculate address for message
  uint16_t addr = ADDR_MESSAGE_LIST + offset;

  //Write message object data (UUID, payload, length)
  uint8_t buffer[13];
  //UUID 5 Byte
  memcpy(buffer, message.getFrom(), 5);
  memcpy(buffer + 5, message.getTo(), 5);
  //Payload 2 Byte
  buffer[10] = (uint8_t)(message.getPayload() & 0xFF);    //LSB
  buffer[11] = (uint8_t)(message.getPayload >> 8);    //MSB
  //Length 1 Byte
  buffer[12] = message.getLength();

  //Write 13 Byte buffer to EEPROM
  for(int i = 0; i < 13; i++) {
    EEPROM::write(addr + i, buffer[i]);
  }

  //Update message pointer offset for next save
  unsigned short next_offset = (offset + 13) % (MAX_MESSAGES * 13);
  //Store Index
  EEPROM::write(ADDR_MESSAGE_OFFSET, next_offset / 13);

  return true;
}
