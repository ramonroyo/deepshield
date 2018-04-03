// Copyright © ByteHeed.  All rights reserved.
#![allow(non_camel_case_types, non_snake_case, dead_code)]

use super::winapi::shared::minwindef::{LPVOID, ULONG, LPHANDLE, USHORT};
use super::winapi::shared::ntdef::{HANDLE};
use std::mem;

type ULONG64 = u64;
type SIZE_T = usize;

pub trait RawStruct<T> {
    fn init() -> T {
        let s: T = unsafe { mem::zeroed() };
        s
    }

    fn size(&self) -> usize {
        mem::size_of::<T>()
    }

    fn as_ptr(&self) -> LPVOID {
        self as *const Self as LPVOID
    }

    fn as_mut_ptr(&mut self) -> LPVOID {
        self as *mut Self as LPVOID
    }
}

#[derive(Debug, Clone, Copy)]
pub enum MapMode {
    KernelMode,
    UserMode
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub enum TestResult {
    TestSuccess,
    TestErrorNoMemory,
    TestErrorRequestInvalid,
    TestErrorDetectionFailed,
    TestErrorUnknown
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub enum TestRequest {
    TestRdtscDetection = 1,
    TestDummyRdtscDetection,
    TestUnknown
}

STRUCT!{
    #[derive(Debug)]
    struct DP_TEST_RDTSC_DETECTION  {
        Request: TestRequest,
        Result: TestResult,
}}

impl RawStruct<DP_TEST_RDTSC_DETECTION> for DP_TEST_RDTSC_DETECTION  { }
