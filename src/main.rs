use core::num;
use std::u8;
extern crate spidev;
extern crate gpio_cdev;
extern crate font8x8;

use std::{thread,time};
use std::io;
use std::io::prelude::*;
use spidev::{Spidev, SpidevOptions, SpiModeFlags};
use gpio_cdev::{Line,Chip, LineRequestFlags};

use font8x8::{BASIC_FONTS, UnicodeFonts};
fn create_spi() -> io::Result<Spidev> {
    let mut spi = Spidev::open("/dev/spidev6.0")?;
    let options = SpidevOptions::new()
         .bits_per_word(8)
         .max_speed_hz(40_000_000)
         .mode(SpiModeFlags::SPI_MODE_0)
         .build();
    spi.configure(&options)?;
    Ok(spi)
}

pub struct St7920Glcd{
spi : Spidev,
cs : Line,
graphic : [u8;1042],
fb1: [[bool; 128]; 64],
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
   self.spi.write(&[0xf8+(0<<1)]).unwrap();
   self.spi.write(&[cmd&0xf0]).unwrap();       // send the higher nibble first
   self.spi.write(&[(cmd<<4)&0xf0]).unwrap();  // send the lower nibble
   self.delay(50);
   self.cs.request(LineRequestFlags::OUTPUT, 0, "blinky").unwrap();

}

fn send_data(&mut self,data:u8){

	self.write_pin(true);  // PUll the CS high
	self.spi.write(&[0xf8+(1<<1)]).unwrap();  // send the SYNC + RS(1)
	self.spi.write(&[data&0xf0]).unwrap();  // send the higher nibble first
	self.spi.write(&[(data<<4)&0xf0]).unwrap();  // send the lower nibble
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
/*
fn to_normal_mode(&mut self){

}
*/
pub fn set_pixel(&mut self,x:i32,y:i32,set:bool)->bool{
        if (x<=128) && (y<=64)
        {
            self.fb1[y as usize][x as usize] = set;
            return true
        }
        return false
}

fn assemble_graphic_buffer(&mut self){
    let pixels_per_buffer = 8; // 8 for serial, 16 for parallel
    let mut num_pixels =0;  
    let mut current_buffer =0;
    let mut temp_buffer:u16 = 0xFFFF;
    // Assemble into 16 columns with 32 rows -> ram_buf[[u8;16],32]
    // First 32 columns are mapped into ram buffers 0-7
    /*
    ---------------------------------
    |0  |                       |15
    |16 |                       |
    |32 |                       |512|
    ---------------------------------
    |513  |                     |
    | ..  |                     |
    |     |                     |1023|
    ----------------------------------
     */
    for zeile in 0..32{
        for ram_buffer in 0..16{
    
                for pixel in 0..8{
                    let t_spalte = ram_buffer*8 + pixel;  // ram_buffer0:pixel 0-15, ram_buffer1: pixel 16-32
                    temp_buffer = temp_buffer | ((self.fb1[zeile][t_spalte]as u16) << (7-pixel)); 
                }
                self.graphic[current_buffer] = (temp_buffer as i32 & 0x00FF) as u8 ;
                current_buffer +=1;
                temp_buffer = 0;
            }
        }
    
    for zeile in 32..64{
        for ram_buffer in 0..16{
                for pixel in 0..8{
                    let t_spalte = ram_buffer*8 + pixel; 
                    temp_buffer = temp_buffer | ((self.fb1[zeile][t_spalte]as u16) << (7-pixel)); 
                }
                self.graphic[current_buffer] = (temp_buffer as i32 & 0x00FF) as u8 ;
                current_buffer +=1;
                temp_buffer = 0;
            }
        } 
        
}

pub fn draw_fb(&mut self){
  let mut buffer_index;	
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

// Here we are going to get comfy:
// Trait: Set a character or string, let the interface draw pixels and buffers
trait CharacterDisplay {
    fn draw_string(&mut self,x:i32,y:i32,str:&str);
    fn highlight(&self,ab:(i32,i32),yz:(i32,i32));
}

impl CharacterDisplay for St7920Glcd{
fn draw_string(&mut self,x:i32,y:i32,str:&str) {
    
    // Bei newline in naechste "Zeile" wechseln
    // Wenn x> 128 wird! in naechste "Zeile" wechseln
    for i in 0..str.len(){
        if let Some(glyph) = BASIC_FONTS.get(str.chars().nth(i).unwrap()) {
            for row in &glyph {
                for bit in 0..8 {
                    match *row & 1 << bit {
                        0 => {print!(" ");self.set_pixel(x+i as i32*8+bit,y+*row as i32 ,false);},
                        _ => {print!("█");self.set_pixel(x+i as i32*8+bit,y+*row as i32 ,true);},
                        
                    }
                }
                println!() 
    }
}
    }


}

fn highlight(&self,ab:(i32,i32),yz:(i32,i32)) {
    
}
}
fn main() {
    println!("Hello, world!");
    let buf : [u8;1042] = [0x0;1042];
    let fb : [[bool;128];64] = [[false;128];64];

    let mut fb2: [[bool;128];64] = [[false;128];64]; // Matrix "spalte mal zeile"
    let mut buf2: [u8;1042] = [0x0;1042];
    // Rahmen ums display malen:
    for zeile in 0..64
    {
        fb2[zeile][0]   = true;
        fb2[zeile][127] = true;
    }
    for spalte in 0..128
    {
        fb2[0] [spalte] = true;
        fb2[63][spalte] = true;
    } 


    // Assemble into 16 columns with 32 rows -> ram_buf[[u8;16],32]
    // First 32 columns are mapped into ram buffers 0-7
    /*
    ---------------------------------
    |0  |                       |15
    |16 |                       |
    |32 |                       |512|
    ---------------------------------
    |513  |                     |
    | ..  |                     |
    |     |                     |1023|
    ----------------------------------
     */
/* // This works as expected!
    let pixels_per_buffer = 8; // 8 for serial, 16 for parallel
    let mut num_pixels =0;  
    let mut current_buffer =0;
    let mut temp_buffer:u16 = 0xFFFF;
    let mut t_pixel =0;

for zeile in 0..32{
    for ram_buffer in 0..16{

            for pixel in 0..8{
                t_pixel = 7-pixel;
                let t_spalte = ram_buffer*8 + pixel;  // ram_buffer0:pixel 0-15, ram_buffer1: pixel 16-32
                temp_buffer = temp_buffer | ((fb2[zeile][t_spalte]as u16) << t_pixel); 
            }
            buf2[current_buffer] = (temp_buffer as i32 & 0x00FF) as u8 ;
            //buf2[current_buffer+1] = ((temp_buffer as i32 & 0xFF00) >>8) as u8;
            current_buffer +=1;
            temp_buffer = 0;
        }
    }

for zeile in 32..64{
    for ram_buffer in 0..16{
            for pixel in 0..8{
                t_pixel = 7-pixel;
                let t_spalte = ram_buffer*8 + pixel; 
                temp_buffer = temp_buffer | ((fb2[zeile][t_spalte]as u16) << t_pixel); 
            }
            buf2[current_buffer] = (temp_buffer as i32 & 0x00FF) as u8 ;
            //buf2[current_buffer+1] = ((temp_buffer as i32 & 0xFF00) >>8) as u8;
            current_buffer +=1;
            temp_buffer = 0;
        }
    } 
*/


// Writing to Ram:
/*
1. Set vertical address（Y）for GDRAM  -> 0-32
2. Set horizontal address（X）for GDRAM  -> 0-8
3. Write D15〜D8 to GDRAM (first byte) 
4. Write D7〜D0 to GDRAM (second byte)
*/


  
    let spi6 = create_spi().unwrap();
    let mut chip = Chip::new("/dev/gpiochip0").unwrap();
    let handle = chip
        .get_line(16).unwrap();
 //       .request(LineRequestFlags::OUTPUT, 1, "blinky")?;

    let mut glcd = St7920Glcd{
        spi: spi6,
        cs: handle,
        graphic: buf2,
        fb1:fb2,
    };

    glcd.init();
    glcd.to_graphic_mode();
    glcd.draw_fb();
    glcd.assemble_graphic_buffer();

    let now = std::time::Instant::now();
    glcd.draw_fb();
    let elapsed_time = now.elapsed();
    println!("Running slow_function() took {} ms.", elapsed_time.as_millis());

    let unicode = '\u{0041}';
        if let Some(glyph) = BASIC_FONTS.get('Q' as char) {
            for x in &glyph {
                for bit in 0..8 {
                    match *x & 1 << bit {
                        // Spalte für Spalte printen
                        0 => print!("{} ",bit),
                        _ => print!("{}█",bit),
                    }
                }
                // Wenn wir hier ankommen, wird die zeile gewechselt
                println!("Row {}",x)
            }
        }

        let pangram: &str = "aBc";
//        glcd.draw_string(0,0,pangram);
//        glcd.draw_fb();
}

