/**
 * This sketch programs the microcode EEPROMs for the 8-bit breadboard computer
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
#define ADDI 0b1001 << 4
#define SUB 0b0011 << 4
#define SUBI 0b1010 << 4
#define STA 0b0100 << 4
#define LDI 0b0101 << 4
#define JMP 0b0110 << 4
#define JC  0b0111 << 4
#define JZ  0b1000 << 4
#define JMPI 0b1011 << 4
#define JMPA 0b1100 << 4
#define OUT 0b1110 << 4
#define HLT 0b1111 << 4


// count by 1
#define COUNTER 14
byte program0[] = {
  LDI | 1,
  STA | COUNTER,
  OUT,
  ADD | COUNTER,
  JMP | 2
};

// memory locations to use for variables
#define X 0
#define Y 1

// count by value in 0x00
byte program1[] = {
  LDI | 0,
  OUT,
  ADD | X,
  JMP | 1
};

// fibonacci
byte program2[] = {
  LDI | 0,
  STA | 15, // prev = 0
  LDI | 1,
  STA | 14, // curr = 1
  // LOOP
  ADD | 15, // A = A + prev
  JC  | 0,  // if > 255, start over
  OUT,
  STA | 13, // tmp = curr + prev
  LDA | 14, // A = curr
  STA | 15, // prev = A (curr)
  LDA | 13, // A = tmp
  STA | 14, // curr = A (curr + prev)
  JMP | 4   // goto LOOP
};

// multiply value in 0x00 (X) by 0x01 (Y)
// non-destructive - does not overwrite the input variables
// requires custom SUBI (subtract immediate) command
//  keep running total in 0x10
//  keep constant 1 in 0xFF
#define RESULT 2

byte program3[] = {
  LDI | 0,
  STA | RESULT,  // result = 0
  LDA | Y,
  STA | COUNTER, // counter = y
  // LOOP
  SUBI | 1, // A=X-1
  JC  | 9,  // goto NOT_DONE (subtraction sets carry, unless it wraps)
  LDA | RESULT,  // A=result
  OUT,      // show result
  HLT,      // exit
  // NOT_DONE
  STA | COUNTER,  // counter = counter - 1
  LDA | RESULT,  // A=result
  ADD | X,  // A=result + first factor
  STA | RESULT,  // result = A (result + second factor)
  LDA | COUNTER,  // load counter
  JMP | 4,  // goto LOOP 
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


/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents(int start, int length) {
  for (int base = start; base < length; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

byte rom[1024] = {0};

void setup() {

  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  Serial.begin(57600);

  // Program data bytes
  Serial.print("Programming EEPROM");

  byte progID = 0b00;
  for (int i=0;i<sizeof(program0); i++){
    rom[ (progID << 8) + i] = program0[i];
  }
   progID = 0b01;
  for (int i=0;i<sizeof(program1); i++){
    rom[ (progID << 8) + i] = program1[i];
  }
  progID = 0b10;
  for (int i=0;i<sizeof(program2); i++){
    rom[ (progID << 8) + i] = program2[i];
  }
  progID = 0b11;
  for (int i=0;i<sizeof(program3); i++){
    rom[ (progID << 8) + i] = program3[i];
  }

  // Program the 8 high-order bits of microcode into the first 128 bytes of EEPROM
  for (int address = 0; address < 1024; address += 1) {
    // write rom here
    writeEEPROM(address, rom[address]);
    
    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done");


  // Read and print out the contents of the EERPROM
  Serial.println("Reading EEPROM");
  printContents(0, 2048);
}


void loop() {
  // put your main code here, to run repeatedly:

}
