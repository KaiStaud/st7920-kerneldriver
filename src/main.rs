use std::u8;
extern crate spidev;
extern crate gpio_cdev;
use std::{thread,time};
use std::io;
use std::io::prelude::*;
use spidev::{Spidev, SpidevOptions, SpidevTransfer, SpiModeFlags};
use gpio_cdev::{Line,Chip, LineRequestFlags};

fn create_spi() -> io::Result<Spidev> {
    let mut spi = Spidev::open("/dev/spidev6.0")?;
    let options = SpidevOptions::new()
         .bits_per_word(8)
         .max_speed_hz(20_000)
         .mode(SpiModeFlags::SPI_MODE_0)
         .build();
    spi.configure(&options)?;
    Ok(spi)
}

pub struct St7920Glcd{
spi : Spidev,
cs : Line,
graphic : [u8;1042]
// fb1: [[u8; 128]; 64],
// fb2: [[u8; 128]; 64]
}

impl St7920Glcd {

fn delay(&self,us:u64){
    let timespan = time::Duration::from_micros(us);
    thread::sleep(timespan);
}

fn write_pin(&mut self,set:bool){
self.cs.request(LineRequestFlags::OUTPUT,set as u8,"blinky").unwrap();
}

fn send_cmd(&mut self,cmd:u8) {

   self.cs.request(LineRequestFlags::OUTPUT, 1, "blinky").unwrap();
   self.spi.write(&[0xf8+(0<<1)]);
   self.spi.write(&[cmd&0xf0]);       // send the higher nibble first
   self.spi.write(&[(cmd<<4)&0xf0]);  // send the lower nibble
   self.delay(50);
   self.cs.request(LineRequestFlags::OUTPUT, 0, "blinky").unwrap();
   /*
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);  // PUll the CS high
	SendByteSPI(0xf8+(0<<1));  // send the SYNC + RS(0)
	SendByteSPI(cmd&0xf0);  // send the higher nibble first
	SendByteSPI((cmd<<4)&0xf0);  // send the lower nibble
	//self.delay(50);
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);  // PUll the CS LOW
*/
}

fn send_data(&mut self,data:u8){
	self.write_pin(true);  // PUll the CS high
	self.spi.write(&[0xf8+(1<<1)]);  // send the SYNC + RS(1)
	self.spi.write(&[data&0xf0]);  // send the higher nibble first
	self.spi.write(&[(data<<4)&0xf0]);  // send the lower nibble
	self.delay(50);
	self.write_pin(false);  // PUll the CS LOW

}

fn init(&mut self){
    self.write_pin(false);  // RESET=0
	self.delay(10000);   // wait for 10ms
    self.write_pin(true);  // RESET=1
    self.delay(50000);   //wait for >40 ms
	self.send_cmd(0x30);  // 8bit mode
	self.delay(110);  //  >100us delay
	self.send_cmd(0x30);  // 8bit mode
	self.delay(40);  // >37us delay
	self.send_cmd(0x08);  // D=0, C=0, B=0
	self.delay(110);  // >100us delay
	self.send_cmd(0x01);  // clear screen
    self.delay(12000);  // >10 ms delay
	self.send_cmd(0x06);  // cursor increment right no shift
	self.delay(1000);  // 1ms delay
	self.send_cmd(0x0C);  // D=1, C=0, B=0
    self.delay(1000);  // 1ms delay
	self.send_cmd(0x02);  // return to home
    self.delay(1000);  // 1ms delay
}

pub fn to_graphic_mode(&mut self){
    self.send_cmd(0x30);  // 8 bit mode
    self.delay (1000);
    self.send_cmd(0x34);  // switch to Extended instructions
    self.delay (1000);
    self.send_cmd(0x36);  // enable graphics
    self.delay (1000);
}

fn to_normal_mode(&mut self){

}

pub fn set_pixel(&mut self,x:i32,y:i32,set:bool)->bool{
        true
}

pub fn draw_fb(&mut self){
  let mut buffer_index = 2*8+16*16;	
   for y in 0..64
	{
		if y <32 
		{
			for x in 0..8	// Draws top half of the screen.
			{			
                // In extended instruction mode, vertical and horizontal coordinates must be specified before sending data in.
				self.send_cmd(0x80 | y);	// Vertical coordinate of the screen is specified first. (0-31)
				self.send_cmd(0x80 | x);	// Then horizontal coordinate of the screen is specified. (0-8)
				buffer_index  = 2*x as u32+16*y as u32 ;
				self.send_data(self.graphic[buffer_index as usize]);	// Data to the upper byte is sent to the coordinate.
				self.send_data(self.graphic[(buffer_index+1) as usize]);	// Data to the lower byte is sent to the coordinate.
			}
		}
		else
		{
			for x in 0..8	// Draws bottom half of the screen.
			{
              // Actions performed as same as the upper half screen.
			  self.send_cmd(0x80 | (y-32));// Vertical coordinate must be scaled back to 0-31 as it is dealing with another half of the screen.
			  self.send_cmd(0x88 | x);
			buffer_index  = 2*x as u32+16*y as u32 ;
			  self.send_data(self.graphic[(buffer_index) as usize]);
			  self.send_data(self.graphic[(buffer_index+1)as usize]);
			}
        }
    }
}

}

fn main() {
    println!("Hello, world!");
    let mut buf : [u8;1042] = [0x0;1042];
    let spi6 = create_spi().unwrap();
    let mut chip = Chip::new("/dev/gpiochip0").unwrap();
    let handle = chip
        .get_line(16).unwrap();
 //       .request(LineRequestFlags::OUTPUT, 1, "blinky")?;
    let mut glcd = St7920Glcd{
        spi: spi6,
        cs: handle,
        graphic: buf,
    };
    glcd.init();
    glcd.to_graphic_mode();
    glcd.set_pixel(0,0,true);
    glcd.draw_fb();
for i in glcd.graphic
    {
            glcd.graphic[i as usize] = 0x00;
    }
    glcd.delay(2000000);
    glcd.draw_fb();

}
