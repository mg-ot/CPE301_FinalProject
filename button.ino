//Authors: Manuel Gonzalez, Tirth Patel, Vanessa Medina, Luis Matheus

#define RDA 0x80
#define TBE 0x20 

// Timer1 registers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned int *)  0x84;
volatile unsigned char *myTIFR1  = (unsigned char *) 0x36;

// Port B registers (PB6, PB7 output)
volatile unsigned char *portB     = (unsigned char *) 0x25;
volatile unsigned char *portDDRB  = (unsigned char *) 0x24;

// Port E registers (PE4 = INT4 input)
volatile unsigned char *portE     = (unsigned char *) 0x2E;
volatile unsigned char *portDDRE  = (unsigned char *) 0x2D;

volatile unsigned char *myEICRB   = (unsigned char *) 0x6A;
volatile unsigned char *myEIMSK   = (unsigned char *) 0x3D;

// System states
enum SystemState {
  DISABLED = 0,
  IDLE     = 1,
  RUNNING  = 2,
  ERROR    = 3
};

volatile uint8_t state = IDLE;

void setupButtonTimer() {
    // Set PB6 and PB7 as output
    *portDDRB |= (1 << 6) | (1 << 7);
    
    // Timer1 setup
    *myTCCR1A = 0x00;
    *myTCCR1B = 0x00;           
    *myTCCR1C = 0x00;
    *myTIMSK1 |= (1 << 0);  
    *myTCNT1 = 0;

    // PE4 (INT4) as input (Pin 2)
    *portDDRE &= ~(1 << 4); 
    *portE    |=  (1 << 4);  

    //falling edge trigger
    *myEICRB &= ~(0x0F);     // Clear ISC40/ISC41 bits
    *myEICRB |= (1 << 2);   
    *myEIMSK |= (1 << 4);    
}

// External Interrupt ISR
ISR(INT4_vect) {
    *myTCNT1 = 0;
    *myTCCR1B = (1 << 1); 
}

// Timer1 Overflow ISR
ISR(TIMER1_OVF_vect) {
    *myTCCR1B &= 0xF8;

    if (state == IDLE) {
        *portB |=  (1 << 6);  
        *portB &= ~(1 << 7);   
        state = DISABLED; //Here put logic to stop the mmotor and sensors
        
    } else {
        *portB &= ~(1 << 6);   
        *portB |=  (1 << 7);   
        state = IDLE; // From idle it can go to Ruuning
    }

    *myTCNT1 = 0; 
}

void setup() {
    *portDDRB |= 0xC0;  
    *portB    &= ~0xC0; 
    state = IDLE;
    setupButtonTimer();
    sei();            
}

void loop() {
    // All logic handled by interrupts
}
