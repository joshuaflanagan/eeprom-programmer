/**
 * This sketch programs the software ROM EEPROMs for the 8-bit breadboard computer
 * It includes support for a flags register with carry and zero flags
 * See this video for more: https://youtu.be/Zg1NdPKoosU
 */
#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

#define NOP 0b0000 << 4
#define LDA 0b0001 << 4
#define ADD 0b0010 << 4
#define SUB 0b0011 << 4
#define STA 0b0100 << 4
#define LDI 0b0101 << 4
#define JMP 0b0110 << 4
#define JC  0b0111 << 4
#define JZ  0b1000 << 4
#define ADI 0b1001 << 4
#define SUI 0b1010 << 4
#define OUT 0b1110 << 4
#define HLT 0b1111 << 4

// - the 4 high bytes are the operation
// - the 4 low bytes are the argument

// count by 1
byte program0[] = {
  LDI | 1,
  OUT,
  ADI | 1,
  JMP | 1
};

// count by 5
byte program1[] = {
  HLT
};

// fibonacci sequence
byte program2[] = {
  HLT
};

byte program3[] = {
  HLT
};



/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setAddress(int address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}


/*
 * Read a byte from the EEPROM at the specified address.
 */
byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}


/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  setAddress(address, /*outputEnable*/ false);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, OUTPUT);
  }

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }
  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);
  delay(10);
}

void print16bytes(int address, byte data[16]){
    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            address, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
}

/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents(int start, int length) {
  for (int base = start; base < (start + length); base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    print16bytes(base, data);
  }
}

byte rom[1024] = {0};
  
void setup() {
  Serial.begin(57600); // make sure Serial Monitor uses same baud
  Serial.println("Press Enter to program EEPROM:");
  while (Serial.available() < 1){
    delay(200);
  }
  Serial.read();

  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);

  // Program data bytes
  Serial.print("Programming EEPROM");

  // There are 11 address pins
  // The most significant bit (A10) toggles between regular output or inverted output
  // The next two lower bits (A8-A9) determine which program is running (0 - 3)
  // The other 8 bits determining the step in the program. For now, since our program
  // counter is limited to 4 bits, we will only use the lowest 4 bits (A0-A3). 
  byte progID;

  // I=inverted, PP=progID, xxxx=ignored, ssss=step
  // I PP xxxx ssss

  // Copy the programs into the 0-initialized rom variable
  
  progID = 0b00;
  for(int i=0; i<sizeof(program0); i++){
    rom[(progID << 8) + i] = program0[i];
  }

  progID = 0b01;
  for(int i=0; i<sizeof(program1); i++){
    rom[(progID << 8) + i] = program1[i];
  }

  progID = 0b10;
  for(int i=0; i<sizeof(program2); i++){
    rom[(progID << 8) + i] = program2[i];
  }

  progID = 0b11;
  for(int i=0; i<sizeof(program3); i++){
    rom[(progID << 8) + i] = program3[i];
  }


  Serial.println();


  // Write out the programs, in the lower 1024 bytes (A10=0)
  for (int address = 0; address < 1024; address += 1){
    writeEEPROM(address, rom[address]);
    
    if (address % 64 == 0) {
      Serial.print(".");
    }
  }

  // Write out the inverted bits, in the upper 1024 bytes (A10=1)
  for (int address = 0; address < 1024; address += 1){
    writeEEPROM(address + 1024, ~rom[address]);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }

  Serial.println(" done");

  Serial.println("Reading EEPROM");
  printContents(0, 2048);
}


void loop() {
  // put your main code here, to run repeatedly:

}
