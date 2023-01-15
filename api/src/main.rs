#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_parens)]
#![allow(unused_mut)]

use std::env;

// https://docs.rs/serde_json/latest/serde_json/
//use serde_json::json;
use serde::{Deserialize, Serialize};
use serde_json::Result;




#[derive(Serialize, Deserialize)]
struct Person {
    name: String,
    age: u8,
    phones: Vec<String>,
}








fn main()
{





    //let p: Person = serde_json::from_str(data)?;








}
