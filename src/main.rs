use std::u8;

pub struct St7920Glcd{
// fb1: [[u8; 128]; 64],
// fb2: [[u8; 128]; 64]

}

impl St7920Glcd {
    
pub fn new(&self) -> Self{
    St7920Glcd { }
}    

fn send_cmd(&self,cmd:u8) {
    
	/*
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);  // PUll the CS high

	SendByteSPI(0xf8+(0<<1));  // send the SYNC + RS(0)
	SendByteSPI(cmd&0xf0);  // send the higher nibble first
	SendByteSPI((cmd<<4)&0xf0);  // send the lower nibble
	//delay_us(50);
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);  // PUll the CS LOW
*/
}

fn send_data(&self,data:u8){
/*/	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);  // PUll the CS high

	SendByteSPI(0xf8+(1<<1));  // send the SYNC + RS(1)
	SendByteSPI(data&0xf0);  // send the higher nibble first
	SendByteSPI((data<<4)&0xf0);  // send the lower nibble
	//delay_us(50);
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);  // PUll the CS LOW
*/
}

fn init(&self){
//    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);  // RESET=0
	//HAL_Delay(10);   // wait for 10ms
//	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);  // RESET=1
//HAL_Delay(50);   //wait for >40 ms
	self.send_cmd(0x30);  // 8bit mode
	//delay_us(110);  //  >100us delay
	self.send_cmd(0x30);  // 8bit mode
	//delay_us(40);  // >37us delay
	self.send_cmd(0x08);  // D=0, C=0, B=0
	//delay_us(110);  // >100us delay
	self.send_cmd(0x01);  // clear screen
//HAL_Delay(12);  // >10 ms delay
	self.send_cmd(0x06);  // cursor increment right no shift
	//HAL_Delay(1);  // 1ms delay
	self.send_cmd(0x0C);  // D=1, C=0, B=0
    //HAL_Delay(1);  // 1ms delay
	self.send_cmd(0x02);  // return to home
	//HAL_Delay(1);  // 1ms delay
}

pub fn to_graphic_mode(&self){
    self.send_cmd(0x30);  // 8 bit mode
    //HAL_Delay (1);
    self.send_cmd(0x34);  // switch to Extended instructions
    //HAL_Delay (1);
    self.send_cmd(0x36);  // enable graphics
    //HAL_Delay (1);
}

fn to_normal_mode(&self){

}

pub fn set_pixel(&self,x:i32,y:i32,set:bool)->bool{
    true
}

}
fn main() {
    println!("Hello, world!");
    let glcd = St7920Glcd{

    };
    glcd.init();
    glcd.to_graphic_mode();
    glcd.set_pixel(0,0,true);
}
