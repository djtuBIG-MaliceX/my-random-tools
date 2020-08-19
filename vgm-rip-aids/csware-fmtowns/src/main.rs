/**
 * 2020 James Alan Nguyen
 * http://www.codingchords.com
 * 
 * Binpack rotator for FMTOWNS C's ware games
 * 
 * Tested:  XENON, DESIRE
 * 
 * thought i'd use this as an excuse to give rust a go. still noob
 * 
 * usage:
 *   CsWareRotate <in_file> <out_file>
 * 
 *   cargo run <in_file> <out_file>
 */

use std::fs::File;
use std::io::prelude::*;
use std::vec::Vec;
use std::slice;
use std::mem;
use std::env;

#[derive(Debug, Clone, Copy)]
#[repr(packed)]
struct HeaderRow
{
    filename: [u8;0xC],
    offset: u32
}

#[repr(packed)]
struct CsWareType
{
    header_size: u16,
    file_row: Vec<HeaderRow>,
    songdata: Vec<Vec<u8>>
}

// taken from https://stackoverflow.com/questions/28127165/how-to-convert-struct-to-u8
fn any_as_u8_slice<T: Sized>(p: &T) -> &[u8] {
    unsafe {
        slice::from_raw_parts(
            (p as *const T) as *const u8,
            mem::size_of::<T>(), 
        )
    }
}

// based on implementation taken from https://www.reddit.com/r/rust/comments/dw2vb3/convert_from_u8_to_generic_sized_struct/
fn convert_slice_u8_to_ref_type<T>(buf: &[u8]) -> &T {
    let p: *const T = buf.as_ptr() as *const T;
    unsafe { &*p }
 }

fn main() -> std::io::Result<()>
{
    let args: Vec<String> = env::args().collect();
    let in_filename: &str = if args.len() >= 2 {&*args[1] } else { "M" };
    let out_filename: &str = if args.len() >= 3 { &*args[2] } else { "M-NEW" };
    
    let mut buffer = Vec::<u8>::new();
    {
        let mut file = File::open(in_filename)?;
        file.read_to_end(&mut buffer)?;
    }
    
    let mut input_data = CsWareType
    {
        header_size: 0,
        file_row: Vec::<HeaderRow>::new(),
        songdata: Vec::<Vec<u8>>::new()
    };

    // read header 
    let mut pos: usize = 2;
    input_data.header_size = *convert_slice_u8_to_ref_type(&buffer[0..pos]);
    
    for _ in 0..(input_data.header_size/16)
    {
        let mut x = HeaderRow
        {
            filename: [0; 0xC],
            offset: 0
        };
        
        x.filename = *convert_slice_u8_to_ref_type(&buffer[pos..pos+0xC]);
        pos += 0xC;
        x.offset = *convert_slice_u8_to_ref_type(&buffer[pos..pos+0x4]);
        pos += 0x4;
        
        unsafe
        {
            //println!("{:?}", String::from_utf8_lossy(&x.filename));
            input_data.file_row.push(x);
        }
    }

    // read song data
    let mut idx: usize = 0;
    unsafe
    {
        let mut x:usize;
        let mut y:usize;
            
        loop
        {
            x = input_data.file_row[idx].offset as usize;
            y = input_data.file_row[idx+1].offset as usize;
            
            let song: Vec<u8> = buffer[x..y].iter().cloned().collect();
            println!("{}: {} to {} len: {}, actual: {}", idx, x, y, y-x, song.len());
            input_data.songdata.push(song);
            idx+=1;
            if idx+1 == input_data.file_row.len()
            {
                break;
            }
        }
    }

    // rewrite new data rotated by one place.
    let mut new_data: Vec<HeaderRow> = Vec::<HeaderRow>::new();
    let mut output: Vec<Vec<u8>> = Vec::<Vec<u8>>::new();

    unsafe
    {
        idx = 0;
        let old_offset: u32 = input_data.file_row[0].offset; 
        let mut lsize: u32 = old_offset.clone();

        loop
        {
            if idx+2 == input_data.file_row.len()
            {
                //new_data.push(input_data.file_row[0].clone());
                output.push(input_data.songdata.remove(0));
                
            } else
            {
                //new_data.push(input_data.file_row[idx+1].clone());
                output.push(input_data.songdata.remove(1));
            }
            new_data.push(input_data.file_row[idx].clone());
            new_data[idx].offset = lsize;
            
            println!("{}: offset {}   len: {}", idx, new_data[idx].offset, output[output.len()-1].len());

            lsize += output[idx].len() as u32;
            idx+=1;

            if idx+1 == input_data.file_row.len()
            {
                new_data.push(input_data.file_row[idx].clone());
                new_data[idx].offset = lsize;
                break;
            }
        }
    }
    
    // now write shit
    {
        let mut fout = File::create(out_filename)?;
        unsafe
        {
            // header
            fout.write_all(&any_as_u8_slice(&input_data.header_size))?;

            loop
            {
                let h: HeaderRow = new_data.remove(0);
                let b: &[u8] = any_as_u8_slice(&h);
                fout.write_all(&b)?;
                if new_data.len() == 0
                {
                    break;
                }
            }
            
            // data
            loop
            {
                let d: Vec<u8> = output.remove(0);
                fout.write_all(&d)?;
                if output.len() == 0
                {
                    break;
                }
            }
        }
    }
    return Ok(());
}