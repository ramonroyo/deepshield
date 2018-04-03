use super::clap::{App, ArgMatches, SubCommand};
use super::failure::Error;

use super::DeviceName;
use super::iochannel::Device;
use super::io::{test_deepshield, TestRequest};

use std::sync::mpsc::Sender;
use super::cli::output::ShellMessage;

pub fn _not_implemented_subcommand(_matches: &ArgMatches, _messenger: &Sender<ShellMessage>) {
    unimplemented!()
}

fn _not_implemented_command(_messenger: &Sender<ShellMessage>) {
    unimplemented!()
}

pub fn bind() -> App<'static, 'static> {
    SubCommand::with_name("test")
        .about("test driver features")
        .version("0.1")
        .author("Sherab G. <sherab.giovannini@byteheed.com>")
        .subcommand(SubCommand::with_name("rdtsc")
            .subcommand(SubCommand::with_name("detection").about("executes rdtsc detection test"))
            .subcommand(SubCommand::with_name("stress").about("executes rdtsc detection stress")))
}



#[allow(unused_variables)]
fn test_rdtsc_detection(messenger: &Sender<ShellMessage>) {
    println!("opening {}", DeviceName);

    let device = Device::new(DeviceName).expect("Can't open deepshield");

    let test = TestRequest::TestDummyRdtscDetection;

    println!("starting RDTSC {:?}", test);

    let result = test_deepshield(&device, test)
                                    .expect("error calling test");

    println!("Result: {:?}", result);
}

pub fn parse_rdtsc(matches: &ArgMatches, messenger: &Sender<ShellMessage>) -> Result<(), Error> {
    match matches.subcommand() {
        ("detection", Some(_))  => test_rdtsc_detection(messenger),
        _                       => println!("{}", matches.usage())
    }

    Ok(())
}

pub fn parse(matches: &ArgMatches, messenger: &Sender<ShellMessage>) -> Result<(), Error> {
    match matches.subcommand() {
        ("rdtsc", Some(matches))  => parse_rdtsc(matches, messenger)?,
        _                   => println!("{}", matches.usage())
    }

    Ok(())
}
