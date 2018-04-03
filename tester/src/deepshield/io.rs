// Copyright © ByteHeed.  All rights reserved.

use super::iochannel::{ Device, IoCtl };

use super::byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};

use super::winapi::um::{winioctl};

use super::iochannel::error::DeviceError;
use super::failure::Error;
pub use super::structs::{TestRequest, TestResult,
                     DP_TEST_RDTSC_DETECTION, RawStruct};

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

pub fn test_deepshield(device: &Device,
                       request: TestRequest) -> Result<TestResult, Error> {

    let control = IoCtl::new(Some("DP_TEST_RDTSC_DETECTION"),
                             IOCTL_DEEPSHIELD_TYPE,
                             0x0A20,
                             Some(winioctl::METHOD_BUFFERED),
                             Some(winioctl::FILE_ANY_ACCESS));

    let mut test = DP_TEST_RDTSC_DETECTION::init();

    test.Request = request;

    let (ptr, len) = (test.as_ptr(), test.size());

    device.raw_call(control, ptr, len)?;

    Ok(test.Result)
}
