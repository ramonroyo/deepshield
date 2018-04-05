#![feature(iterator_step_by, asm)]
#[allow(unused_imports)]
#[macro_use] extern crate failure;
#[macro_use] extern crate failure_derive;
// Deprecated in favor of ShellMessage
//#[macro_use] extern crate slog;
#[macro_use] extern crate bitflags;
#[macro_use] mod ffi;
// #[macro_use] extern crate enum_primitive;
extern crate indicatif;

pub mod cli;
pub mod iochannel;
pub mod service;
pub mod deepshield;
