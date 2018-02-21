from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject
from m5.objects import CommMonitor
from MemObject import MemObject
from Device import DmaDevice

# frame size
# 4k
# full: 24883200B
# encoded: 777600B (32:1)
# 1080P
# full : 12441600B
# encoded: 388800B (32:1)

# processing latency per cache line(64B)
# decoder : 12 / 4665600, 2332800
# encoder : 18 / 6998400, 3499200 
# camera :  16 / 6220800, 3110400
# network : 1 / 388800, 1944--
# display : 1 / 

class LGGovernor(MemObject):
    type = 'LGGovernor'
    cxx_header = "LG/lg_governor.hh"

    system = Param.System(Parent.any, "System Object")
    frame_per_seconds = Param.Int(60, "Frame Per Seconds of Multimedia System")
    setup_delay = Param.Int(1000, "Initialization delay")
    # flow_type = Param.String("Flow(Application type of Multimedia System")
    video_decoder = Param.LGVideoDecoder("Video Decoder")
    video_camera = Param.LGVideoCamera("Video Camera")
    video_encoder = Param.LGVideoEncoder("Video Encoder")
    video_display = Param.LGVideoDisplay("Display Controller")
    video_nw = Param.LGVideoNW("Video Network Module")
    response_queue = Param.IPPacketQueue("Wrapping class of response queue")
    flow_type = Param.String('YouTube', "Flow Type")
    sliceMode = Param.Bool("false", "Enable Slice mode")
    full_frame_size_in = Param.MemorySize('24883200B', "Size of decoded frame")
    full_frame_size_out = Param.MemorySize('6220800B', "Size of decoded frame")
    encoded_frame_size_in = Param.MemorySize('777600B', "Size of encoded frame")
    encoded_frame_size_out = Param.MemorySize('194400B', "Size of encoded frame")
    totalSliceCount_in = Param.Int(1, "Number of slices in frame")
    totalSliceCount_out = Param.Int(1, "Number of slices in frame")

class LGVideoDecoder(MemObject):
    type = 'LGVideoDecoder'
    cxx_header = "LG/lg_video_decoder.hh"

    system = Param.System(Parent.any, "System Object")
    response_queue = Param.IPPacketQueue("Wrapping class of response queue")
    input_buffer_size = Param.MemorySize('64kB', "Size of input buffer")
    output_buffer_size = Param.MemorySize('64kB', "Size of output buffer")
    full_frame_size = Param.MemorySize('24883200B', "Size of decoded frame")
    encoded_frame_size = Param.MemorySize('3110400B', "Size of encoded frame")
    processing_latency = Param.Int(12, "Decoding time per cache line")
    coding_ratio = Param.Int(8, "Video Coding Ratio")
    read_port = MasterPort("Dma port")
    write_port = MasterPort("Dma port")
    
class LGVideoCamera(MemObject):
    type = 'LGVideoCamera'
    cxx_header = "LG/lg_video_cam.hh"
	
    system = Param.System(Parent.any, "System Object")
    response_queue = Param.IPPacketQueue("Wrapping class of response queue")
    input_buffer_size = Param.MemorySize('64kB', "Size of input buffer")
    output_buffer_size = Param.MemorySize('64kB', "Size of output buffer")
    full_frame_size = Param.MemorySize('24883200B', "Size of decoded frame")
    encoded_frame_size = Param.MemorySize('3110400B', "Size of encoded frame")
    processing_latency = Param.Int(16, "Decoding time per cache line")
    coding_ratio = Param.Int(8, "Video Coding Ratio")
    write_port = MasterPort("Dma port")
    
class LGVideoEncoder(MemObject):
    type = 'LGVideoEncoder'
    cxx_header = "LG/lg_video_encoder.hh"

    system = Param.System(Parent.any, "System Object")
    response_queue = Param.IPPacketQueue("Wrapping class of response queue")
    input_buffer_size = Param.MemorySize('64kB', "Size of input buffer")
    output_buffer_size = Param.MemorySize('64kB', "Size of output buffer")
    full_frame_size = Param.MemorySize('24883200B', "Size of decoded frame")
    encoded_frame_size = Param.MemorySize('3110400B', "Size of encoded frame")
    processing_latency = Param.Int(18, "Decoding time per cache line")
    coding_ratio = Param.Int(8, "Video Coding Ratio")
    read_port = MasterPort("Dma port")
    write_port = MasterPort("Dma port")
    
class LGVideoDisplay(MemObject):
    type = 'LGVideoDisplay'
    cxx_header = "LG/lg_video_dc.hh"

    system = Param.System(Parent.any, "System Object")
    response_queue = Param.IPPacketQueue("Wrapping class of response queue")
    input_buffer_size = Param.MemorySize('64kB', "Size of input buffer")
    output_buffer_size = Param.MemorySize('64kB', "Size of output buffer")
    full_frame_size = Param.MemorySize('24883200B', "Size of decoded frame")
    encoded_frame_size = Param.MemorySize('3110400B', "Size of encoded frame")
    processing_latency = Param.Int(16, "Decoding time per cache line")
    coding_ratio = Param.Int(8, "Video Coding Ratio")
    read_port = MasterPort("Dma port")
    
class LGVideoNW(MemObject):
    type = 'LGVideoNW'
    cxx_header = "LG/lg_video_nw.hh"

    system = Param.System(Parent.any, "System Object")
    buffer_size = Param.MemorySize('256kB', "Size of input/output buffer")
    full_frame_size = Param.MemorySize('24883200B', "Size of decoded frame")
    encoded_frame_size = Param.MemorySize('777600B', "Size of encoded frame")
    setup_delay = Param.Int(1000, "Initialization delay")
    processing_latency = Param.Int(388800, "Decoding time per cache line")
    sleep_latency= Param.Int(1000, "Number of cycles to go sleep")
    wakeup_latency= Param.Int(1000, "Number of cycles to wake up")
    dma_port = MasterPort("Dma port")
    response_queue = Param.IPPacketQueue("Wrapping class of response queue")

class IPPacketQueue(SimObject):
    type = 'IPPacketQueue'
    cxx_header = "LG/ip_packet_queue.hh"
