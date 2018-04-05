use super::failure::Error;
use super::clap::{App, ArgMatches, SubCommand};
use super::process;
use super::winreg::RegKey;
use super::winreg::enums::*;
use std::path::Path;

use std::sync::mpsc::Sender;
use super::cli::output::{ShellMessage};

fn _not_implemented_command(_messenger: &Sender<ShellMessage>) -> Result<(), Error> {
    unimplemented!()
}

/*
* DeepShield requires an OperationMode once it is installed, so this is just to ensure
* we always have this
*/
fn create_registry_parameter() -> Result<(), Error>{
    let hklm = RegKey::predef(HKEY_LOCAL_MACHINE);
    let subkey = "SYSTEM\\CurrentControlSet\\services\\deepshield";
    let path = Path::new(subkey).join("Parameters");

    let key = hklm.create_subkey(&path)?;

    hklm.open_subkey(subkey)?;

    key.set_value("OperationMode", &40u32)?;

    Ok(())
}

pub fn parse(matches: &ArgMatches, messenger: &Sender<ShellMessage>) -> Result<(), Error> {
    let service_name = "deepshield";
    let service_filename = "dpshield";

    let action: &Fn(&str, &str, &Sender<ShellMessage>) = match matches.subcommand_name() {
        Some("install") => { &super::functions::install },
        Some("remove")  => { &super::functions::remove },
        Some("update")  => { &super::functions::update },
        Some("start")   => { &super::functions::start },
        Some("stop")    => { &super::functions::stop },
        Some("query")   => { &super::functions::query },
        Some("run")     => {
            super::functions::reinstall(service_name, service_filename, messenger);
            super::functions::start(service_name, service_filename, messenger);
            create_registry_parameter()?;
            return Ok(());
        },
        _               => {
            println!("{}", matches.usage());
            process::exit(0)
        }

    };

    action(service_name, service_filename, messenger);

    create_registry_parameter()?;

    Ok(())
}

pub fn bind() -> App<'static, 'static> {
    SubCommand::with_name("service")
        .about("service controller for deepshield")
        .version("0.1")
        .author("Sherab G. <sherab.giovannini@byteheed.com>")
        .subcommand(SubCommand::with_name("install").about("installs dpshield.sys"))
        .subcommand(SubCommand::with_name("remove").about("deletes service"))
        .subcommand(SubCommand::with_name("update").about("reinstalls service"))
        .subcommand(SubCommand::with_name("start").about("starts service"))
        .subcommand(SubCommand::with_name("query").about("query service"))
        .subcommand(SubCommand::with_name("stop").about("stops service"))
        .subcommand(SubCommand::with_name("run").about("stops, reinstalls and starts service"))
}
