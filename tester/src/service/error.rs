use std::io::Error;

#[derive(Fail, Debug)]
pub enum ServiceError {
    #[fail(display = "Unable to {} service: ({}) ({})", _0, _1, _2)]
    Control(String, String, String),
    #[fail(display = "Error parsing: {} ({})", _0, _1)]
    Parse(String, #[cause] Error),
}
