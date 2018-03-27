use super::clap::{App, ArgMatches, SubCommand};
use super::failure::Error;

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



pub fn parse(matches: &ArgMatches, messenger: &Sender<ShellMessage>) -> Result<(), Error> {
    match matches.subcommand() {
        ("features", Some(_)) | ("region", Some(_))  => _not_implemented_command(messenger),
        _                                            => println!("{}", matches.usage())
    }

    Ok(())
}
