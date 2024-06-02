#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4

#define ADDR_0 8
#define ADDR_3 11
#define CLOCK 5
#define ADDR_IN 6
#define RAM_IN 7

#define NOP 0b00000000
#define LDA 0b00010000
#define ADD 0b00100000
#define SUB 0b00110000
#define STA 0b01000000
#define LDI 0b01010000
#define JMP 0b01100000
#define JC  0b01110000
#define JZ  0b10000000
#define OUT 0b11100000
#define HLT 0b11110000

void setup() {
  Serial.begin(57600);

  digitalWrite(SHIFT_CLK, LOW);
  digitalWrite(ADDR_IN, HIGH);
  digitalWrite(RAM_IN, HIGH);
  digitalWrite(CLOCK, HIGH);
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);

  pinMode(CLOCK, OUTPUT);
  pinMode(ADDR_IN, OUTPUT);
  pinMode(RAM_IN, OUTPUT);
  
  for (int pin = ADDR_0; pin <= ADDR_3; pin += 1) {
    pinMode(pin, OUTPUT);
  }

  writeRAM(0, LDA | 14);
  writeRAM(1, SUB | 12);
  writeRAM(2, JC | 6);
  writeRAM(3, LDA | 13);
  writeRAM(4, OUT);
  writeRAM(5, HLT);
  writeRAM(6, STA | 14);
  writeRAM(7, LDA | 13);
  writeRAM(8, ADD | 15);
  writeRAM(9, STA | 13);
  writeRAM(10, JMP | 0);
  writeRAM(11, NOP);
  writeRAM(12, 1);
  writeRAM(13, 0); //sum
  writeRAM(14, 3);
  writeRAM(15, 7);
  // for some reason, need to set this again...
  writeRAM(0, LDA | 14);

  setAddress(0xFF);
  setDataPins(0xFF);
  Serial.println("Done");  
}

void writeRAM(byte address, byte data){
  setAddress(address);
  setDataPins(data);
  latchData();
}

void setAddress(byte address){
  for (int pin = ADDR_0; pin <= ADDR_3; pin += 1) {
    digitalWrite(pin, address & 1);
    address = address >> 1;
  }
}

void latchData(){
  digitalWrite(CLOCK, LOW);
  //delay(100);
  digitalWrite(CLOCK, HIGH);
}

void setDataPins(byte data){
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, data);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

void loop() {
  //byte data[1] = {0};
  byte addr = 255;
  int count = Serial.available();

  while (count > 0){
    byte val = Serial.read();
    if (val >= 48 && val <= 57){
      // digit 0-9
      addr = val - 48;
    } else if (val >= 97 && val <= 102){
      // char a-f
      addr = val - 87;
    } else {
      addr = 255;
    }

    if (addr < 255){
      Serial.print("Setting address ");
      Serial.println(addr, HEX);
      setAddress(addr);
    }
    count = Serial.available();
  }
}
