// Copyright © ByteHeed.  All rights reserved.

use super::iochannel::{ Device, IoCtl };

use super::byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};

use super::iochannel::error::DeviceError;
use super::failure::Error;
use std::io::Cursor;

use std::{mem, fmt};

pub const IOCTL_DEEPSHIELD_TYPE: u32 = 0xB080;
pub const SE_NT_DEVICE_NAME: &str = "\\\\.\\Deepshield";

enum_from_primitive! {
    #[derive(Debug, Clone)]
    enum PartitionOption {
        None = 0,
        TraceDebugEvents,
        TraceToDisk,
        CoalesceNotifications,
        CollectStats,
        SecureMode,
    }
}
