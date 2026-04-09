#!/usr/bin/env python3

"""
HackRF SIMD DSP USB Test Script

This script tests the SIMD DSP functionality via USB vendor requests.
It requires a HackRF device with SIMD-enabled firmware.
"""

import sys
import time
import struct
import argparse

try:
    import hackrf
except ImportError:
    print("Error: hackrf Python library not found")
    print("Install with: pip install hackrf")
    sys.exit(1)

# Vendor request definitions for SIMD DSP
HACKRF_VENDOR_REQUEST_DSP_SIMD_ENABLE = 0x44
HACKRF_VENDOR_REQUEST_DSP_SIMD_FILTER_CONFIG = 0x45
HACKRF_VENDOR_REQUEST_DSP_SIMD_IQ_CONFIG = 0x46
HACKRF_VENDOR_REQUEST_DSP_SIMD_BENCHMARK = 0x47
HACKRF_VENDOR_REQUEST_DSP_SIMD_STATUS = 0x48
HACKRF_VENDOR_REQUEST_DSP_TEST_RUN = 0x49
HACKRF_VENDOR_REQUEST_DSP_TEST_DATA = 0x4A
HACKRF_VENDOR_REQUEST_DSP_TEST_VALIDATE = 0x4B

class HackRFDSPTester:
    def __init__(self):
        self.device = None
        self.serial = None
        
    def open(self):
        """Open connection to HackRF device"""
        try:
            result = hackrf.init()
            if result != hackrf.HACKRF_SUCCESS:
                raise Exception(f"Failed to initialize hackrf library: {result}")
                
            result = hackrf.open_by_serial(self.serial, self.device)
            if result != hackrf.HACKRF_SUCCESS:
                raise Exception(f"Failed to open device: {result}")
                
            print("HackRF device opened successfully")
            return True
            
        except Exception as e:
            print(f"Error opening device: {e}")
            return False
    
    def close(self):
        """Close connection to HackRF device"""
        if self.device:
            hackrf.close(self.device)
        hackrf.exit()
        print("HackRF device closed")
    
    def vendor_request(self, request_type, value=0, index=0, data=None, length=0):
        """Send vendor request to HackRF device"""
        try:
            if data is None:
                data = bytearray(length)
                
            result = hackrf.vendor_request(self.device, request_type, value, index, data, length)
            if result != hackrf.HACKRF_SUCCESS:
                raise Exception(f"Vendor request failed: {result}")
                
            return bytes(data)
            
        except Exception as e:
            print(f"Vendor request error: {e}")
            return None
    
    def enable_simd(self, enable=True):
        """Enable or disable SIMD DSP processing"""
        print(f"{'Enabling' if enable else 'Disabling'} SIMD DSP...")
        value = 1 if enable else 0
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_SIMD_ENABLE, value)
        return result is not None
    
    def configure_filter(self, filter_type=0, num_taps=8):
        """Configure FIR filter"""
        print(f"Configuring filter: type={filter_type}, taps={num_taps}")
        # filter_type in low byte, num_taps in high byte
        value = (num_taps << 8) | filter_type
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_SIMD_FILTER_CONFIG, value)
        return result is not None
    
    def configure_iq_correction(self, offset_i=100, offset_q=-50):
        """Configure I/Q correction"""
        print(f"Configuring I/Q correction: I_offset={offset_i}, Q_offset={offset_q}")
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_SIMD_IQ_CONFIG, offset_i, offset_q)
        return result is not None
    
    def run_benchmark(self):
        """Run performance benchmark"""
        print("Running performance benchmark...")
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_SIMD_BENCHMARK, length=16)
        
        if result and len(result) == 16:
            # Parse 4 uint32_t values
            fir_improvement, iq_improvement, dot_improvement, overall_improvement = struct.unpack('<4I', result)
            
            print(f"FIR Filter Improvement: {fir_improvement / 100:.2f}x")
            print(f"I/Q Correction Improvement: {iq_improvement / 100:.2f}x")
            print(f"Dot Product Improvement: {dot_improvement / 100:.2f}x")
            print(f"Overall Improvement: {overall_improvement / 100:.2f}x")
            
            return {
                'fir': fir_improvement / 100.0,
                'iq': iq_improvement / 100.0,
                'dot': dot_improvement / 100.0,
                'overall': overall_improvement / 100.0
            }
        else:
            print("Failed to get benchmark results")
            return None
    
    def get_status(self):
        """Get current DSP configuration status"""
        print("Getting DSP status...")
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_SIMD_STATUS, length=4)
        
        if result and len(result) == 4:
            status = struct.unpack('<I', result[0:4])[0]
            
            simd_enabled = bool(status & 0x01)
            filters_enabled = bool(status & 0x02)
            iq_correction_enabled = bool(status & 0x04)
            current_taps = (status >> 8) & 0xFF
            
            print(f"SIMD Enabled: {simd_enabled}")
            print(f"Filters Enabled: {filters_enabled}")
            print(f"I/Q Correction Enabled: {iq_correction_enabled}")
            print(f"Current Filter Taps: {current_taps}")
            
            return {
                'simd_enabled': simd_enabled,
                'filters_enabled': filters_enabled,
                'iq_correction_enabled': iq_correction_enabled,
                'current_taps': current_taps
            }
        else:
            print("Failed to get status")
            return None
    
    def run_firmware_tests(self):
        """Run firmware-side tests"""
        print("Running firmware tests...")
        
        # Quick validation test
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_TEST_RUN, 0, length=1)
        if result and result[0] == 1:
            print("PASS: Quick validation")
        else:
            print("FAIL: Quick validation")
            return False
        
        # Full test suite
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_TEST_RUN, 1, length=1)
        if result and result[0] == 1:
            print("PASS: Full test suite")
        else:
            print("FAIL: Full test suite")
            return False
        
        return True
    
    def get_test_data(self, data_type=0):
        """Get test data from device"""
        print(f"Getting test data type {data_type}...")
        
        if data_type == 0:  # Test signal
            length = 32
        elif data_type == 1:  # Filter coefficients
            length = 16
        elif data_type == 2:  # I/Q samples
            length = 16
        else:
            print("Invalid data type")
            return None
        
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_TEST_DATA, data_type, length=length)
        
        if result:
            if data_type == 0 or data_type == 1:
                # Parse as int16_t array
                data = struct.unpack(f'<{length//2}h', result)
                print(f"Data: {data[:8]}...")  # Show first 8 values
            else:
                # Parse as I/Q samples
                iq_samples = []
                for i in range(0, length, 4):
                    i_val, q_val = struct.unpack('<hh', result[i:i+4])
                    iq_samples.append((i_val, q_val))
                print(f"I/Q samples: {iq_samples[:4]}...")
            
            return result
        else:
            print("Failed to get test data")
            return None
    
    def validate_configuration(self):
        """Validate current DSP configuration"""
        print("Validating DSP configuration...")
        result = self.vendor_request(HACKRF_VENDOR_REQUEST_DSP_TEST_VALIDATE, length=1)
        
        if result:
            validation = result[0]
            simd_works = bool(validation & 0x01)
            streaming_works = bool(validation & 0x02)
            usb_api_works = bool(validation & 0x04)
            
            print(f"SIMD Functions: {'PASS' if simd_works else 'FAIL'}")
            print(f"Streaming Interface: {'PASS' if streaming_works else 'FAIL'}")
            print(f"USB API: {'PASS' if usb_api_works else 'FAIL'}")
            
            return simd_works and streaming_works and usb_api_works
        else:
            print("Failed to validate configuration")
            return False
    
    def run_comprehensive_test(self):
        """Run comprehensive test suite"""
        print("\n" + "="*50)
        print("HackRF SIMD DSP Comprehensive Test")
        print("="*50)
        
        # Test 1: Enable SIMD
        if not self.enable_simd(True):
            print("FAIL: Could not enable SIMD")
            return False
        
        time.sleep(0.1)
        
        # Test 2: Configure filter
        if not self.configure_filter(0, 8):
            print("FAIL: Could not configure filter")
            return False
        
        time.sleep(0.1)
        
        # Test 3: Configure I/Q correction
        if not self.configure_iq_correction(100, -50):
            print("FAIL: Could not configure I/Q correction")
            return False
        
        time.sleep(0.1)
        
        # Test 4: Get status
        status = self.get_status()
        if not status:
            print("FAIL: Could not get status")
            return False
        
        # Test 5: Validate configuration
        if not self.validate_configuration():
            print("FAIL: Configuration validation failed")
            return False
        
        # Test 6: Run firmware tests
        if not self.run_firmware_tests():
            print("FAIL: Firmware tests failed")
            return False
        
        # Test 7: Run benchmark
        benchmark = self.run_benchmark()
        if not benchmark:
            print("FAIL: Benchmark failed")
            return False
        
        # Test 8: Get test data
        test_data = self.get_test_data(0)
        if not test_data:
            print("FAIL: Could not get test data")
            return False
        
        print("\n" + "="*50)
        print("ALL TESTS PASSED!")
        print("="*50)
        print(f"Performance Improvement: {benchmark['overall']:.2f}x")
        print("SIMD DSP is working correctly!")
        
        return True

def main():
    parser = argparse.ArgumentParser(description='Test HackRF SIMD DSP functionality')
    parser.add_argument('--serial', help='HackRF device serial number')
    parser.add_argument('--test', choices=['quick', 'comprehensive', 'benchmark'], 
                       default='comprehensive', help='Test type to run')
    parser.add_argument('--enable', action='store_true', help='Enable SIMD DSP')
    parser.add_argument('--disable', action='store_true', help='Disable SIMD DSP')
    parser.add_argument('--status', action='store_true', help='Show DSP status')
    parser.add_argument('--benchmark', action='store_true', help='Run performance benchmark')
    
    args = parser.parse_args()
    
    tester = HackRFDSPTester()
    tester.serial = args.serial
    
    if not tester.open():
        sys.exit(1)
    
    try:
        if args.enable:
            tester.enable_simd(True)
        elif args.disable:
            tester.enable_simd(False)
        elif args.status:
            tester.get_status()
        elif args.benchmark:
            tester.run_benchmark()
        elif args.test == 'quick':
            success = tester.enable_simd(True) and tester.validate_configuration()
            print("QUICK TEST: " + ("PASS" if success else "FAIL"))
        elif args.test == 'comprehensive':
            success = tester.run_comprehensive_test()
            sys.exit(0 if success else 1)
        elif args.test == 'benchmark':
            benchmark = tester.run_benchmark()
            if benchmark:
                print(f"Overall improvement: {benchmark['overall']:.2f}x")
            else:
                sys.exit(1)
    
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"Test error: {e}")
        sys.exit(1)
    
    finally:
        tester.close()

if __name__ == '__main__':
    main()
