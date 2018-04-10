// Copyright © ByteHeed.  All rights reserved.

extern crate failure;
extern crate clap;
extern crate slog;
extern crate winapi;
extern crate console;
extern crate winreg;

use super::cli;
use super::ffi;
use std::process;

#[macro_use] mod macros;
mod consts;
mod structs;
mod core;
mod functions;
mod error;


pub mod command;


pub use self::functions::*;

pub use self::structs::ServiceStatus;
pub use self::core::{WindowsService, ServiceStatusError};
