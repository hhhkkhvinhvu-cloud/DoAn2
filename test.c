#include <16f877a.h>
#use delay(crystal=20000000)

// Cau hinh I2C
#use i2c(Master, Fast, sda=PIN_C4, scl=PIN_C3) 

unsigned int8 code[10]={0XC0,0XF9,0XA4,0XB0,0X99,0X92,0X82,0XF8,0X80,0X90};

// DINH NGHIA THOI GIAN
CONST INT8 RED=23, GREEN=20, YELLOW=3;
CONST INT8 RED2=60, GREEN2=57, YELLOW2=3;

// BIEN TOAN CUC
INT8 I1=0, I2=0; 
// --- SU DUNG INT16 DE KHONG BI TRAN SO KHI DEM DEN 400 ---
UNSIGNED INT16 COUNT_TICK=0; 
// ---------------------------------------------------------
UNSIGNED INT8 mode=0; 
UNSIGNED INT8 COUNTDOWN=0, COUNTDOWN2=0;
int1 update_time_flag = 0; 
int1 blink_state = 0;      
unsigned int8 scan_idx = 0; 

// BIEN RTC
int1 che_do_tu_dong = 1; 
unsigned int8 rtc_sec, rtc_min, rtc_hour;

unsigned int8 bcd_to_decimal(unsigned int8 bcd) {
   return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// DOC RTC 
void DOC_THOIGIAN() {
   output_float(PIN_C3); output_float(PIN_C4); // Reset bus I2C ao
   
   i2c_start();
   i2c_write(0xD0);
   i2c_write(0x00);
   i2c_start();
   i2c_write(0xD1);
   rtc_sec = bcd_to_decimal(i2c_read(1));
   rtc_min = bcd_to_decimal(i2c_read(1));
   rtc_hour = bcd_to_decimal(i2c_read(0));
   i2c_stop();
}

// CAP NHAT TRANG THAI DEN GIAO THONG
VOID CAPNHAT_DEN(){ 
   output_low(PIN_E0); output_low(PIN_E1); output_low(PIN_E2);
   output_low(PIN_B0); output_low(PIN_B1); output_low(PIN_B2);

   if(mode == 2) { // Che do dem
      if(blink_state) { output_high(PIN_E1); output_high(PIN_B1); }
      return;
   }

   // Cum 1 (E0-E2)
   if(I1 == 0) output_high(PIN_E0);      
   else if(I1 == 1) output_high(PIN_E1); 
   else output_high(PIN_E2);             

   // Cum 2 (B0-B2)
   if(I2 == 3) output_high(PIN_B0);      
   else if(I2 == 4) output_high(PIN_B1); 
   else output_high(PIN_B2);             
}

// ================= NGAT TIMER 1 =================
#INT_TIMER1
VOID ISR(){
   // Timer set chay khoang 2.5ms
   set_timer1(64000); 
   
   // --- PHAN 1: QUET LED 7 DOAN ---
   output_low(PIN_A0); output_low(PIN_A1); output_low(PIN_A2); output_low(PIN_A3);
   
   if(mode != 2) { 
       switch(scan_idx) {
           case 0: // Hang chuc den 1
               output_d(code[COUNTDOWN/10]); output_high(PIN_A0); 
               break;
           case 1: // Hang don vi den 1
               output_d(code[COUNTDOWN%10]); output_high(PIN_A1); 
               break;
           case 2: // Hang chuc den 2
               output_d(code[COUNTDOWN2/10]); output_high(PIN_A2); 
               break;
           case 3: // Hang don vi den 2
               output_d(code[COUNTDOWN2%10]); output_high(PIN_A3); 
               break;
       }
   }
   scan_idx++;
   if(scan_idx > 3) scan_idx = 0;

   // --- PHAN 2: DEM THOI GIAN ---
   COUNT_TICK++; // Bay gio no la INT16, dem thoai mai len 400
   
   // Nhap nhay den vang
   if(COUNT_TICK % 200 == 0) blink_state = !blink_state;

   // 1 GIAY (400 ticks * 2.5ms = 1000ms)
   if(COUNT_TICK >= 400){
     COUNT_TICK=0; 
     update_time_flag = 1; 
     
     if(mode != 2){ 
         if(COUNTDOWN > 0) COUNTDOWN--;  
         if(COUNTDOWN2 > 0) COUNTDOWN2--;   
         
         // Logic chuyen mau
         if(COUNTDOWN == 0){
            I1++; if(I1 > 2) I1 = 0;
            if(I1==0) COUNTDOWN = (mode==1)? GREEN2 : GREEN;
            else if(I1==1) COUNTDOWN = (mode==1)? YELLOW2 : YELLOW;
            else COUNTDOWN = (mode==1)? RED2 : RED;
         }
         
         if(COUNTDOWN2 == 0){
            I2++; if(I2 > 5) I2 = 3;
            if(I2==3) COUNTDOWN2 = (mode==1)? GREEN2 : GREEN;
            else if(I2==4) COUNTDOWN2 = (mode==1)? YELLOW2 : YELLOW;
            else COUNTDOWN2 = (mode==1)? RED2 : RED;
         }
     }
   }
   CAPNHAT_DEN();
}

// XU LY NUT BAM
#INT_RB
VOID XULY_NUTBAM(){   
   if(!input(PIN_B7)){ 
      che_do_tu_dong = !che_do_tu_dong; 
      if(!che_do_tu_dong) { mode=0; I1=0; I2=5; COUNTDOWN=GREEN; COUNTDOWN2=RED; }
   }
   if(che_do_tu_dong == 0) {
       if(!input(PIN_B4)){ mode=0; I1=0; I2=5; COUNTDOWN=GREEN; COUNTDOWN2=RED; }
       if(!input(PIN_B5)){ mode=1; I1=0; I2=5; COUNTDOWN=GREEN2; COUNTDOWN2=RED2; }
       if(!input(PIN_B6)){ mode=2; }
   }
   clear_interrupt(INT_RB); 
}   

void main(){
   setup_adc_ports(NO_ANALOGS); // Quan trong
   
   set_tris_a(0x00);
   set_tris_b(0xF0); 
   set_tris_d(0x00);
   set_tris_e(0x00);
   
   OUTPUT_A(0); OUTPUT_D(0); OUTPUT_E(0); OUTPUT_B(0);

   mode = 0; I1 = 0; I2 = 5;
   COUNTDOWN = GREEN; COUNTDOWN2 = RED;

   // Setup Timer 1 voi bo chia 8
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8); 
   set_timer1(64000); 
   
   enable_interrupts(INT_TIMER1);
   enable_interrupts(INT_RB);
   enable_interrupts(GLOBAL);
   
   while(true){
      if(update_time_flag){
         update_time_flag = 0; 
         
         if(che_do_tu_dong){
             DOC_THOIGIAN(); 
             
             if ((rtc_hour >= 6 && rtc_hour < 8) || (rtc_hour >= 16 && rtc_hour < 18)) {
                if(mode != 1) { mode = 1; I1=0; I2=5; COUNTDOWN=GREEN2; COUNTDOWN2=RED2; }
             }
             else if (rtc_hour >= 22 || rtc_hour < 5) {
                if(mode != 2) { mode = 2; }
             }
             else {
                if(mode != 0) { mode = 0; I1=0; I2=5; COUNTDOWN=GREEN; COUNTDOWN2=RED; }
             }
         }
      }
   }
}
