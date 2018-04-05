use std::io::Error;

#[derive(Fail, Debug)]
pub enum DeepshieldError {
    #[fail(display = "DeepShield I/O ({}) ({})", _0, _1)]
    IoCall(String, #[cause] Error),
    #[fail(display = "Error parsing: {} ({})", _0, _1)]
    Parse(String, #[cause] Error),
}
