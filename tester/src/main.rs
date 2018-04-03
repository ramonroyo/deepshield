// Copyright © ByteHeed.  All rights reserved.
use conveyor::{iochannel, service, deepshield};

extern crate clap;
extern crate conveyor;
extern crate failure;
extern crate slog_term;
extern crate termcolor;

use failure::Error;

use std::process;
use clap::{App, Arg, ArgMatches, SubCommand};
use std::sync::mpsc::channel;
use std::sync::mpsc::{Sender};
use conveyor::cli::output::{create_messenger, MessageType, ShellMessage};

fn run(app: &ArgMatches, messenger: &Sender<ShellMessage>) -> Result<(), Error> {
    match app.subcommand() {
        ("services", Some(matches)) => service::command::parse(matches, &messenger),
        _ => Ok(println!("{}", app.usage())),
    }
}

fn main() {
    print!(
        "\n      .;:
     ::::
     ; ;:
       ;:
   ::: ;::::::::  :##### ##  ## ###### #####  ;,   #  ####@ ##### ;####
   ::  ;:     ::  :#   #  #  #    ##   ##     ;,   #  #     #     ;.   #
   ::  ;:     ::  :#   #  ####    ##   ##     ;.   #  #     #     ;.   #
   ::  ;:     ::  :#####  `##`    ##   #####  ;#####  ####; ####@ ;.   #
   ::  ;:     ::  :#   #   ##     ##   ##     ;,   #  #     #     ;.   #
   ::         ::  :#   #   ##     ##   ##     ;,   #  #     #     ;.   #
   ::         ::  :#####   ##     ##   #####  ;,   #  ##### ##### ;####
   ::;;;;;;;;;::

                                        :::::::::::::::::::::::::::::::::::
                                        :: www.byteheed.com/microdefense ::
                                        :::::::::::::::::::::::::::::::::::

Sherab G. <sherab.giovannini@byteheed.com>
A simple testing tool for ironlizard.
___________________________________________________________________________\n\n"
    );
    let target = Arg::with_name("target").short("t")
                            .required(true)
                            .value_name("TARGET")
                            .help("service target");



    let matches = App::new("Ironlizard Tester")
        .about("A gate between humans and dragons.")
        .version("1.0")
        .author("Sherab G. <sherab.giovannini@byteheed.com>")
        .arg(Arg::with_name("v") .short("v") .multiple(true) .help("Sets the level of verbosity"))
        .subcommand(conveyor::service::command::bind())
        .subcommand(conveyor::deepshield::command::bind())
        .get_matches();

    let (messenger, receiver) = channel();
    let printer = create_messenger(receiver, None, 20);

    if let Err(e) = run(&matches, &messenger) {
        ShellMessage::send( &messenger,
                    format!("Application Error: {}", e), MessageType::Exit, 0, );

        printer.join().expect("Unable to wait for printer");
        process::exit(1);
    }


    ShellMessage::send( &messenger, "".to_owned(), MessageType::Exit, 0, );
    printer.join().expect("Unable to wait for printer");
}
