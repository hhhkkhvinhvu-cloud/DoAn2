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
UNSIGNED INT16 COUNT_TICK=0; 
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
   // Reset bus I2C ao tranh treo
   output_float(PIN_C3); output_float(PIN_C4); 
   
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

   if(mode == 2) { // Che do dem vang
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
   set_timer1(64000); 
   
   // --- QUET LED 7 DOAN ---
   output_low(PIN_A0); output_low(PIN_A1); output_low(PIN_A2); output_low(PIN_A3);
   
   if(mode != 2) { 
       switch(scan_idx) {
           case 0: output_d(code[COUNTDOWN/10]); output_high(PIN_A0); break;
           case 1: output_d(code[COUNTDOWN%10]); output_high(PIN_A1); break;
           case 2: output_d(code[COUNTDOWN2/10]); output_high(PIN_A2); break;
           case 3: output_d(code[COUNTDOWN2%10]); output_high(PIN_A3); break;
       }
   }
   scan_idx++;
   if(scan_idx > 3) scan_idx = 0;

   // --- DEM THOI GIAN ---
   COUNT_TICK++; 
   
   if(COUNT_TICK % 200 == 0) blink_state = !blink_state; // Chop tat vang nhanh

   // 1 GIAY
   if(COUNT_TICK >= 400){
     COUNT_TICK=0; 
     update_time_flag = 1; 
     
     if(mode != 2){ 
         if(COUNTDOWN > 0) COUNTDOWN--;  
         if(COUNTDOWN2 > 0) COUNTDOWN2--;    
         
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

// --- HAM KIEM TRA NUT BAM (MOI) ---
// Su dung Polling thay vi Interrupt de on dinh hon
void KIEMTRA_NUTBAM() {
    // 1. Nut Chuyen Che Do (B7)
    if(!input(PIN_B7)) {
        delay_ms(20); // Chong rung
        if(!input(PIN_B7)) {
            che_do_tu_dong = !che_do_tu_dong; // Dao trang thai
            
            // Neu chuyen sang thu cong thi reset ve mac dinh
            if(!che_do_tu_dong) { 
                mode=0; I1=0; I2=5; COUNTDOWN=GREEN; COUNTDOWN2=RED; 
            }
            // Cho nha phim de khong bi nhay lien tuc
            while(!input(PIN_B7)); 
        }
    }

    // Cac nut con lai chi co tac dung khi che do TU DONG = 0 (Tat)
    if(che_do_tu_dong == 0) {
        // Nut Che do binh thuong (B4)
        if(!input(PIN_B4)) {
            delay_ms(20);
            if(!input(PIN_B4)) {
                mode=0; I1=0; I2=5; COUNTDOWN=GREEN; COUNTDOWN2=RED;
                while(!input(PIN_B4));
            }
        }
        // Nut Che do cao diem (B5)
        if(!input(PIN_B5)) {
            delay_ms(20);
            if(!input(PIN_B5)) {
                mode=1; I1=0; I2=5; COUNTDOWN=GREEN2; COUNTDOWN2=RED2;
                while(!input(PIN_B5));
            }
        }
        // Nut Che do dem (B6)
        if(!input(PIN_B6)) {
            delay_ms(20);
            if(!input(PIN_B6)) {
                mode=2; 
                while(!input(PIN_B6));
            }
        }
    }
}

void main(){
   setup_adc_ports(NO_ANALOGS); 
   
   set_tris_a(0x00);
   set_tris_b(0xF0); // B4-B7 la Input
   set_tris_d(0x00);
   set_tris_e(0x00);
   
   // Keo tro noi len cho Port B (quan trong vi nut bam noi xuong GND)
   port_b_pullups(TRUE); 
   
   OUTPUT_A(0); OUTPUT_D(0); OUTPUT_E(0); OUTPUT_B(0);

   mode = 0; I1 = 0; I2 = 5;
   COUNTDOWN = GREEN; COUNTDOWN2 = RED;

   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8); 
   set_timer1(64000); 
   
   enable_interrupts(INT_TIMER1);
   // enable_interrupts(INT_RB); // --- BO DONG NAY ---
   enable_interrupts(GLOBAL);
   
   while(true){
      // Goi ham quet nut bam lien tuc
      KIEMTRA_NUTBAM();

      if(update_time_flag){
         update_time_flag = 0; 
         
         // Chi doc RTC va tu dong chuyen mode khi bien nay = 1
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
